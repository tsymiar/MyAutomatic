#!/usr/bin/python
# coding: utf-8
"""本地/实时K线图工具 matkline。

数据流：数据源接口 -> 解析成统一7列结构 -> 历史区间过滤 -> 绘图(蜡烛图/折线)

统一结构（绘图只用到第0/2/3/4/5列）:
    [时间, 成交量, 开, 收, 高, 低, 成交额]

数据源（均为免密钥真实接口）:
    tencent      A股/指数/基金    日线(原生支持区间) + 1/5/15/30/60分钟
    eastmoney    东方财富        A股/期货/港股
    gold         沪金/沪银       新浪国内期货，日线全量历史 + 分钟线，真实OHLC
    xau/gc       伦敦金/纽约金    日线真实OHLC；分钟线由当日分时线+实时快照聚合
    crude        纽约原油 WTI    同一机制，日线最早到1996年
    brent        布伦特原油 ICE   同一机制
    ng           美国天然气      同一机制
    usd          美元指数        东财分时聚合出的分钟/小时K线（无免费历史日线）
    binance/okx  加密货币        按 startTime 分页取全

国际盘：`--source xau`(伦敦金) / `gc`(纽约金) / `crude`(WTI原油) / `brent`(布伦特) /
    `ng`(天然气) / `usd`(美元指数)，也支持用中文标的自动选源（--symbol 原油）。
    --interval 1d           真实日线 OHLC（新浪国际期货全量历史）
    --interval 5m/15m/30m/1h 当日分时线聚合，真实高低点，开箱即可用
    --interval 1m           当日每分钟价格，单价的平价K线

交互：悬停蜡烛图看该根K线详情；单击底部副图在 成交量/MACD/RSI/KDJ 之间切换。
导出：`--csv out.csv` 把当前K线写成CSV；指标参数用 --macd/--rsi/--kdj 调整。
"""

import argparse
import csv
import json
import os
import re
import sys
import time
from collections import namedtuple
from datetime import datetime
from functools import partial
from urllib.parse import urlencode
from urllib.request import Request, urlopen

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.offsetbox import AnnotationBbox, HPacker, TextArea, VPacker
from matplotlib.patches import Rectangle
from matplotlib.ticker import FixedLocator, FuncFormatter, MaxNLocator

DEFAULT_FILE = "../WinNTKline/KlineUtil/data/SH600747.DAT"
FILE_ENCODING = "gb2312"
USER_AGENT = "MyAutomatic-matkline/1.0"
TENCENT_REFERER = "https://gu.qq.com/"
SINA_REFERER = "https://finance.sina.com.cn"

# ---------------------------------------------------------------------------
# 一、数据源配置
# ---------------------------------------------------------------------------
SOURCES = {
    "tencent": {
        # web.ifzq.gtimg.cn 会 301 到 web3.ifzq.gtimg.cn（部分网络无法解析），故 ifzq 优先、web 作镜像
        "kline_url": ["https://ifzq.gtimg.cn/appstock/app/fqkline/get",
                      "https://web.ifzq.gtimg.cn/appstock/app/fqkline/get"],
        "minute_url": ["https://ifzq.gtimg.cn/appstock/app/kline/mkline",
                       "https://web.ifzq.gtimg.cn/appstock/app/kline/mkline"],
        "default_symbol": "sh600519",
    },
    "eastmoney": {
        "kline_url": ["https://push2his.eastmoney.com/api/qt/stock/kline/get"],
        "default_symbol": "1.600519",
    },
    "binance": {
        "kline_url": ["https://data-api.binance.vision/api/v3/klines",
                      "https://api.binance.com/api/v3/klines"],
        "default_symbol": "BTCUSDT",
    },
    "okx": {
        "kline_url": ["https://www.okx.com/api/v5/market/candles",
                      "https://aws.okx.com/api/v5/market/candles"],
        "default_symbol": "BTC-USDT",
    },
    "gold": {
        "default_symbol": "AU0",
    },
    "usd": {
        # 美元指数：东财分时接口能稳定取到（最多5个交易日），无免费历史日线
        "default_symbol": "UDI",
        "trends_secid": "100.UDI",
    },
}

# 新浪国际期货（免密钥）：统一走 GlobalFuturesService，
# 日线是真实历史OHLC，分钟线用当日分时线 + 实时快照聚合。
SINA_FUTURES = {
    "xau": {"code": "XAU", "name": "伦敦金现货", "snapshot": "hf_XAU"},
    "gc": {"code": "GC", "name": "纽约金COMEX", "snapshot": "hf_GC"},
    "crude": {"code": "CL", "name": "纽约原油WTI", "snapshot": "hf_CL"},
    "brent": {"code": "OIL", "name": "布伦特原油", "snapshot": "hf_OIL"},
    "ng": {"code": "NG", "name": "美国天然气", "snapshot": "hf_NG"},
}
# 这些源共用同一套加载逻辑，逐条登记进 SOURCES（即 --source 的可选值）
SOURCES.update({name: {"default_symbol": conf["code"], "snapshot_code": conf["snapshot"]}
                for name, conf in SINA_FUTURES.items()})

# 支持别名/中文名选源的数据源集合（贵金属、原油、天然气、美元指数）
COMMODITY_SOURCES = frozenset(SINA_FUTURES) | {"gold", "usd"}

# auto 模式的尝试顺序（逐个容错回退）
AUTO_ORDER = ["tencent", "eastmoney", "binance", "okx"]

# 各数据源单次请求能返回的最大K线根数（--limit 0 或历史区间查询时按此上限取）
SINGLE_REQUEST_MAX = {"tencent": 640, "eastmoney": 10000, "binance": 1000, "okx": 300}

# 周期写法在各数据源的映射
INTERVAL_ALIAS = {"day": "1d", "d": "1d", "1day": "1d", "week": "1w", "w": "1w",
                  "month": "1M", "min": "1m", "1min": "1m", "hour": "1h", "1hour": "1h"}
MINUTE_STEP = {"1m": 1, "5m": 5, "15m": 15, "30m": 30, "60m": 60, "1h": 60}
TENCENT_DAILY_KEY = {"1d": "day", "1w": "week", "1M": "month"}
TENCENT_MINUTE_KEY = {"1m": "m1", "5m": "m5", "15m": "m15", "30m": "m30",
                      "60m": "m60", "1h": "m60"}
EASTMONEY_KLT = {"1m": 1, "5m": 5, "15m": 15, "30m": 30, "60m": 60,
                 "1h": 60, "1d": 101, "1w": 102, "1M": 103}
OKX_BAR = {"1m": "1m", "5m": "5m", "15m": "15m", "30m": "30m", "60m": "1H",
           "1h": "1H", "4h": "4H", "1d": "1D", "1w": "1W", "1M": "1M"}

# 各数据源真正支持的周期（避免 .get(key, default) 静默降级成别的周期）
FUTURES_INTERVALS = ["1m", "5m", "15m", "30m", "60m", "1h", "1d"]    # 新浪期货(国内/国际)
USD_INDEX_INTERVALS = ["1m", "5m", "15m", "30m", "60m", "1h"]         # 东财分时(无免费日线)
SUPPORTED_INTERVALS = {
    "tencent": ["1m", "5m", "15m", "30m", "60m", "1h", "1d", "1w", "1M"],
    "eastmoney": ["1m", "5m", "15m", "30m", "60m", "1h", "1d", "1w", "1M"],
    "binance": ["1m", "3m", "5m", "15m", "30m", "1h", "2h", "4h", "6h", "8h",
                "12h", "1d", "3d", "1w", "1M"],
    "okx": ["1m", "5m", "15m", "30m", "60m", "1h", "4h", "1d", "1w", "1M"],
    "gold": FUTURES_INTERVALS,
    "usd": USD_INDEX_INTERVALS,
}
SUPPORTED_INTERVALS.update({name: FUTURES_INTERVALS for name in SINA_FUTURES})

# 标的形态识别（auto 模式据此挑源）
CRYPTO_SYMBOL = re.compile(r"^[A-Z0-9]{2,12}(?:[-/]?(?:USDT|USDC|USD|BTC|ETH))$")
DOMESTIC_SYMBOL = re.compile(r"^(?:sh|sz|bj|nf_|hf_|[0-9]{6})", re.IGNORECASE)

# 商品/指数别名：--symbol 可直接写中文或常用写法
COMMODITY_ALIASES = {
    # 贵金属
    "au": "AU0", "au0": "AU0", "沪金": "AU0", "黄金": "AU0", "黄金期货": "AU0", "gold": "AU0",
    "xau": "XAU", "xauusd": "XAU", "伦敦金": "XAU", "现货黄金": "XAU", "国际黄金": "XAU",
    "gc": "GC", "gc0": "GC", "comex": "GC", "纽约金": "GC", "纽约黄金": "GC",
    "ag": "AG0", "ag0": "AG0", "沪银": "AG0", "白银": "AG0", "现货白银": "AG0",
    # 能源
    "cl": "CL", "cl0": "CL", "wti": "CL", "crude": "CL", "原油": "CL", "美原油": "CL",
    "纽约原油": "CL", "美国原油": "CL",
    "oil": "OIL", "brent": "OIL", "布伦特": "OIL", "布伦特原油": "OIL",
    "ng": "NG", "ng0": "NG", "天然气": "NG", "美国天然气": "NG",
    # 指数
    "udi": "UDI", "dxy": "UDI", "usdx": "UDI", "usd": "UDI",
    "美元": "UDI", "美元指数": "UDI",
}

# 别名指向的标的 -> 优先数据源（避免「国际黄金」被沪金抢走、「原油」落到股票源）
SYMBOL_SOURCE_ORDER = {
    "AU0": ["gold", "xau", "gc"],     # 沪金主连
    "AG0": ["gold"],                  # 沪银（新浪国内期货）
    "XAU": ["xau", "gc"],             # 伦敦金现货
    "GC": ["gc", "xau"],              # 纽约金 COMEX
    "CL": ["crude"],                  # WTI原油
    "OIL": ["brent"],                 # 布伦特原油
    "NG": ["ng"],                     # 美国天然气
    "UDI": ["usd"],                   # 美元指数
}

# 前缀匹配：别名表没收录的写法（如 au2412、原油2411）也能落到合适的数据源
COMMODITY_PREFIXES = (
    (re.compile(r"^(?:xau|伦敦金|现货黄金|国际黄金)", re.IGNORECASE), ["xau", "gc"]),
    (re.compile(r"^(?:gc0?|comex|纽约金)", re.IGNORECASE), ["gc", "xau"]),
    (re.compile(r"^(?:au0?|ag0?|沪金|沪银|黄金|白银)", re.IGNORECASE), ["gold", "xau", "gc"]),
    (re.compile(r"^(?:cl0?|wti|crude|纽约原油|美国原油|原油)", re.IGNORECASE), ["crude"]),
    (re.compile(r"^(?:oil|brent|布伦特)", re.IGNORECASE), ["brent"]),
    (re.compile(r"^(?:ng0?|天然气)", re.IGNORECASE), ["ng"]),
    (re.compile(r"^(?:udi|dxy|usdx|美元)", re.IGNORECASE), ["usd"]),
)

# 新浪：国内期货(沪金/沪银) 与 国际期货(黄金/原油/天然气) 的K线服务
SINA_INNER_URL = ("https://stock2.finance.sina.com.cn/futures/api/jsonp.php/"
                  "var%%20_%s/InnerFuturesNewService.%s")
SINA_GLOBAL_URL = ("https://stock2.finance.sina.com.cn/futures/api/jsonp.php/"
                   "var%%20_%s/GlobalFuturesService.%s")
SINA_SNAPSHOT_URL = "https://hq.sinajs.cn/list=%s"

# 东财分时(1分钟)行情：美元指数等没有免费历史K线的品种，用它聚合出分钟K线
EASTMONEY_TRENDS_URL = "https://push2his.eastmoney.com/api/qt/stock/trends2/get"

# 中国习惯：红涨绿跌
UP_COLOR = "#d62728"
DOWN_COLOR = "#2ca02c"

# 界面配色：浅灰底 + 白色面板 + 淡网格，长时间看盘不刺眼
STYLE = {
    "figure": "#f4f4f7", "axes": "#ffffff", "grid": "#e4e4ec",
    "edge": "#b9b9c6", "text": "#2b2b33", "muted": "#8b8b9a", "hint": "#a6a6b5",
}
MA_COLORS = ((5, "#e8890c"), (10, "#2f6fd0"), (20, "#8a5cd6"))

# 非交互式后端：不产生窗口，实时刷新时无需（也不应）调用 show()
NON_GUI_BACKENDS = {"agg", "pdf", "ps", "svg", "svgz", "template", "cairo", "pgf"}

# 底部副图：单击可在四种模式间循环切换
PANEL_MODES = ("volume", "macd", "rsi", "kdj")
PANEL_LABEL_FMT = {"volume": "成交量", "macd": "MACD(%d,%d,%d)",
                   "rsi": "RSI(%d)", "kdj": "KDJ(%d,%d,%d)"}
PANEL_HINT = "单击底部副图可在 成交量/MACD/RSI/KDJ 之间切换"

# 指标默认参数，可被 --macd/--rsi/--kdj 覆盖
DEFAULT_PARAMS = {"macd": (12, 26, 9), "rsi": (14,), "kdj": (9, 3, 3)}

# 导出CSV的列顺序（与统一7列结构一一对应）
CSV_HEADER = ("date", "volume", "open", "close", "high", "low", "amount")

# 常见中文字体候选：覆盖 Windows / macOS / Linux 各发行版
CJK_FONT_CANDIDATES = [
    "SimHei", "Microsoft YaHei",              # Windows
    "PingFang SC", "Hiragino Sans GB",        # macOS
    "Noto Sans CJK SC", "Noto Sans CJK JP", "Noto Sans SC", "Source Han Sans SC",
    "WenQuanYi Zen Hei", "WenQuanYi Micro Hei", "Droid Sans Fallback",
    "AR PL UMing CN", "Unifont",              # Linux
]


# ---------------------------------------------------------------------------
# 二、字体
# ---------------------------------------------------------------------------
def _font_available(name):
    """判断 matplotlib 能否真正解析到该字体（不触发 findfont 告警）。"""
    from matplotlib import font_manager
    try:
        font_manager.findfont(font_manager.FontProperties(family=name),
                              fallback_to_default=False)
        return True
    except Exception:
        return False


def setup_font():
    """自动挑选系统已安装的中文字体，避免 SimHei 缺失导致的 findfont 告警与中文乱码。"""
    available = [name for name in CJK_FONT_CANDIDATES if _font_available(name)]
    if not available:
        print("提示：未检测到中文字体，图表中的中文可能显示为方块。\n"
              "      Linux 可安装：sudo apt-get install fonts-wqy-zenhei fonts-noto-cjk",
              file=sys.stderr)
    plt.rcParams["font.family"] = "sans-serif"
    plt.rcParams["font.sans-serif"] = available + ["DejaVu Sans"]
    plt.rcParams["axes.unicode_minus"] = False
    return available


def setup_style():
    """统一整体观感：面板配色、字号、图例与网格默认值。"""
    plt.rcParams.update({
        "figure.facecolor": STYLE["figure"],
        "axes.facecolor": STYLE["axes"],
        "axes.edgecolor": STYLE["edge"],
        "axes.labelcolor": STYLE["text"],
        "axes.titlecolor": STYLE["text"],
        "text.color": STYLE["text"],
        "xtick.color": STYLE["muted"],
        "ytick.color": STYLE["muted"],
        "grid.color": STYLE["grid"],
        "grid.linewidth": 0.6,
        "axes.titlesize": 11.5,
        "axes.labelsize": 9,
        "xtick.labelsize": 8,
        "ytick.labelsize": 8,
        "legend.fontsize": 8,
        "legend.frameon": False,
        "savefig.facecolor": STYLE["figure"],
    })


# ---------------------------------------------------------------------------
# 三、标的与周期归一化
# ---------------------------------------------------------------------------
class UnsupportedInterval(ValueError):
    """数据源不支持所请求的周期；auto 模式下不应被当作取数失败而静默回退。"""


def normalize_interval(interval):
    """把常见周期写法统一成 1m/5m/15m/30m/60m/1h/4h/1d/1w/1M（M=月线，m=分钟）。"""
    text = (interval or "1m").strip()
    if not text:
        return "1m"
    alias = INTERVAL_ALIAS.get(text.lower())
    if alias:
        return alias
    if text[-1] == "M" and text[:-1].isdigit():     # 保留大写的 M，其余统一小写
        return text
    return text.lower()


def validate_interval(source, interval):
    """校验周期是否被该数据源支持，避免静默降级成别的周期。"""
    supported = SUPPORTED_INTERVALS.get(source)
    if supported and interval not in supported:
        raise UnsupportedInterval("%s 不支持周期 %s，可用周期: %s"
                                  % (source, interval, "/".join(supported)))
    return interval


def tencent_code(symbol):
    """把 600519 / sh600519 / sz000001 统一成腾讯的 前缀+代码 形式。"""
    text = symbol.strip().lower()
    if text.startswith(("sh", "sz", "bj", "nf_", "hf_")):
        return text
    digits = "".join(ch for ch in text if ch.isdigit())
    if len(digits) == 6:
        if digits[0] in "69":
            return "sh" + digits
        if digits[0] in "03":
            return "sz" + digits
        if digits[0] in "48":
            return "bj" + digits
        return "sh" + digits
    return text


def eastmoney_secid(symbol):
    """把 600519 / sh600519 统一成东财的 市场.代码 (1=沪, 0=深)。"""
    text = symbol.strip()
    if "." in text and text.split(".")[0] in ("0", "1"):
        return text
    low = text.lower()
    digits = "".join(ch for ch in low if ch.isdigit())
    if len(digits) == 6:
        market = "1" if (digits[0] in "69" or low.startswith("sh")) else "0"
        return "%s.%s" % (market, digits)
    return text


def normalize_symbol(source, symbol):
    """按数据源的代码规范转换标的；未指定标的时用该源默认标的。"""
    if not symbol:
        return SOURCES[source]["default_symbol"]
    text = symbol.strip()
    if source in COMMODITY_SOURCES:
        return COMMODITY_ALIASES.get(text.lower(), text.upper())
    if source == "tencent":
        return tencent_code(text)
    if source == "eastmoney":
        return eastmoney_secid(text)
    if source == "okx":
        upper = text.upper().replace("/", "-")
        if "-" not in upper and upper.endswith("USDT"):
            upper = upper[:-4] + "-USDT"
        return upper
    if source == "binance":
        return text.upper().replace("-", "").replace("/", "")
    return text


def candidate_sources(symbol):
    """auto 模式按标的形态挑源，降低无谓的失败重试。"""
    text = (symbol or "").strip()
    if text:
        target = COMMODITY_ALIASES.get(text.lower())
        if target:
            return list(SYMBOL_SOURCE_ORDER.get(target, ["gold", "xau", "gc"]))
        for pattern, sources in COMMODITY_PREFIXES:
            if pattern.match(text):
                return list(sources)
        if CRYPTO_SYMBOL.match(text.upper()):
            return ["binance", "okx"]
        if DOMESTIC_SYMBOL.match(text):
            return ["tencent", "eastmoney"]
    return list(AUTO_ORDER)


# ---------------------------------------------------------------------------
# 四、历史区间与通用工具
# ---------------------------------------------------------------------------
def parse_date_arg(text):
    """把 YYYY-MM-DD / YYYYMMDD / YYYY/MM/DD 校验并统一成 YYYY-MM-DD。"""
    if text is None or str(text).strip() == "":
        return None
    digits = re.sub(r"[^0-9]", "", str(text))
    if len(digits) != 8:
        raise ValueError("日期格式应为 YYYY-MM-DD 或 YYYYMMDD：%s" % text)
    day = "%s-%s-%s" % (digits[:4], digits[4:6], digits[6:8])
    try:
        datetime.strptime(day, "%Y-%m-%d")
    except ValueError:
        raise ValueError("无效日期：%s" % text)
    return day


def to_timestamp_ms(day, end_of_day=False):
    """把 YYYY-MM-DD 转成毫秒时间戳（与本模块的本地时间格式化保持一致）。"""
    stamp = datetime.strptime(day, "%Y-%m-%d")
    if end_of_day:
        stamp = stamp.replace(hour=23, minute=59, second=59)
    return int(time.mktime(stamp.timetuple())) * 1000


def request_limit(limit, default):
    """limit<=0 表示不限条数，此时退回该数据源单次请求上限。"""
    return limit if limit and limit > 0 else default


def slice_tail(points, limit):
    """只保留最后 limit 根K线，避免超长历史拖慢绘图。limit<=0 表示不限。"""
    if limit and limit > 0 and len(points[0]) > limit:
        return [column[-limit:] for column in points]
    return points


def filter_range(points, start=None, end=None):
    """按 [start, end] 过滤K线（只比较日期部分，兼容 2007/02/02 这类分隔符）。"""
    if not start and not end:
        return points
    keep = []
    for index, when in enumerate(points[0]):
        day = str(when)[:10].replace("/", "-").replace(".", "-")
        if start and day < start:
            continue
        if end and day > end:
            continue
        keep.append(index)
    return [[column[index] for index in keep] for column in points]


FUTURE_TOLERANCE_HOURS = 24


def drop_future_points(points, tolerance_hours=FUTURE_TOLERANCE_HOURS):
    """丢掉时间明显超前于当前时刻的K线。

    新浪国内期货分钟线末端偶尔混入「下一交易日」的占位行（如周六凌晨出现周一 00:00），
    会造成图上多出一根未来K线、且「最新时间」显示成未来日期。日期无法解析时一律保留。
    """
    if not points or not points[0]:
        return points
    horizon = datetime.now().timestamp() + tolerance_hours * 3600
    keep = []
    for index, when in enumerate(points[0]):
        try:
            stamp = datetime.strptime(str(when)[:19], "%Y-%m-%d %H:%M:%S").timestamp()
        except ValueError:
            keep.append(index)               # 日线等无时间的日期串保留原样
            continue
        if stamp <= horizon:
            keep.append(index)
    if len(keep) == len(points[0]):
        return points
    return [[column[index] for index in keep] for column in points]


def shape_points(points, start=None, end=None, limit=0):
    """先按日期区间过滤，再截取末尾 limit 根（limit<=0 表示不限）。"""
    result = filter_range(points, start, end)
    if (start or end) and not result[0]:
        raise ValueError("区间 %s ~ %s 内没有K线数据" % (start or "最早", end or "最新"))
    return slice_tail(result, limit)


def range_suffix(start=None, end=None):
    """生成标题用的区间后缀。"""
    if not start and not end:
        return ""
    return "  %s~%s" % (start or "最早", end or "最新")


def span_text(points):
    """生成数据实际跨度描述。"""
    if not points or not points[0]:
        return "无数据"
    return "%s ~ %s" % (points[0][0], points[0][-1])


def export_csv(points, path):
    """把7列K线写成CSV，返回写入行数。

    用 utf-8-sig 编码写 BOM，Excel 双击打开不会中文乱码；实时模式下每帧覆盖同一个文件。
    """
    target = os.path.abspath(os.path.expanduser(path))
    parent = os.path.dirname(target)
    if parent and not os.path.isdir(parent):
        os.makedirs(parent, exist_ok=True)
    with open(target, "w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(CSV_HEADER)
        writer.writerows(zip(*points))
    return len(points[0])


def write_csv(points, path):
    """导出CSV并打印结果；写失败只提示，不中断绘图。"""
    try:
        print("csv=%s rows=%d" % (path, export_csv(points, path)))
    except OSError as error:
        print("导出CSV失败: %s" % error, file=sys.stderr)


def parse_int_list(text, count, name):
    """把 "12,26,9" 解析成 count 个正整数的元组，用于 --macd/--rsi/--kdj。"""
    items = [part.strip() for part in str(text).split(",")]
    if len(items) != count or not all(part.isdigit() and int(part) > 0 for part in items):
        raise ValueError("%s 需要 %d 个正整数（逗号分隔）：%s" % (name, count, text))
    return tuple(int(part) for part in items)


# ---------------------------------------------------------------------------
# 五、HTTP 取数
# ---------------------------------------------------------------------------
def default_headers():
    return {"User-Agent": USER_AGENT}


def sina_headers():
    """新浪行情接口需要 Referer，否则可能返回空。"""
    return {"User-Agent": USER_AGENT, "Referer": SINA_REFERER}


def fetch_text(url, params=None, timeout=10, encoding="utf-8", headers=None):
    """GET 请求并返回文本。"""
    if params:
        url = url + ("&" if "?" in url else "?") + urlencode(params)
    request = Request(url, headers=headers or default_headers())
    with urlopen(request, timeout=timeout) as response:
        raw = response.read()
    try:
        return raw.decode(encoding)
    except UnicodeDecodeError:
        return raw.decode("utf-8", "ignore")


def fetch_json(url, params=None, timeout=10, encoding="utf-8", headers=None):
    """GET 请求并解析 JSON。"""
    return json.loads(fetch_text(url, params, timeout, encoding, headers))


def fetch_first(urls, params, timeout, headers, parse):
    """依次尝试主地址与镜像地址；parse(payload) 负责解析，全部失败则抛最后一次错误。"""
    last_error = None
    for url in urls:
        try:
            return parse(fetch_json(url, params, timeout, headers=headers))
        except (OSError, ValueError, json.JSONDecodeError, KeyError) as error:
            last_error = error
    raise last_error if last_error else ValueError("没有可用的行情地址")


# ---------------------------------------------------------------------------
# 六、解析器：各数据源 -> 统一7列结构
# ---------------------------------------------------------------------------
def build_points(bars):
    """把 (时间, 开, 收, 高, 低, 量[, 额]) 序列转成 [时间, 量, 开, 收, 高, 低, 额]。

    缺第7项时用收盘价填充成交额（该列仅供展示，绘图不依赖）。
    """
    points = [[], [], [], [], [], [], []]
    for bar in bars:
        points[0].append(bar[0])
        points[1].append(bar[5])
        points[2].append(bar[1])
        points[3].append(bar[2])
        points[4].append(bar[3])
        points[5].append(bar[4])
        points[6].append(bar[6] if len(bar) > 6 else bar[2])
    return points


def normalize_day(stamp):
    """把日期串补齐成 'YYYY-MM-DD HH:MM:SS'（新浪/东财的日期格式略有差异）。"""
    text = str(stamp).strip()
    if len(text) <= 10:
        return text[:10] + " 00:00:00"
    if len(text) == 16:
        return text + ":00"
    return text


def tencent_time(stamp):
    """腾讯: 分钟线 'YYYYMMDDHHMM'，日线及以上 'YYYY-MM-DD'。"""
    text = str(stamp)
    if len(text) >= 12:
        return datetime.strptime(text[:12], "%Y%m%d%H%M").strftime("%Y-%m-%d %H:%M:%S")
    return text.replace("/", "-")[:10] + " 00:00:00"


def millisecond_time(stamp):
    """毫秒/秒时间戳 -> 'YYYY-MM-DD HH:MM:SS'。"""
    stamp = float(stamp)
    if stamp > 1e11:
        stamp /= 1000
    return datetime.fromtimestamp(stamp).strftime("%Y-%m-%d %H:%M:%S")


def parse_jsonp(text):
    """剥离新浪 JSONP 外壳 var _XAU(...);，取出其中的 JSON。"""
    match = re.search(r"\((.*)\)\s*;?\s*$", text, re.S)
    if not match:
        raise ValueError("JSONP 响应格式异常")
    return json.loads(match.group(1))


def parse_tencent(payload, code, interval):
    """腾讯: data[code]['qfqday'] 或 data[code]['m1']，每行 [时间, 开, 收, 高, 低, 量, ...]。"""
    node = payload.get("data")
    if not isinstance(node, dict) or not node:
        raise ValueError("腾讯接口未返回行情数据(%s)" % (payload.get("msg") or "param error"))
    block = node.get(code)
    if block is None:
        block = next(iter(node.values()))
    if not isinstance(block, dict):
        raise ValueError("腾讯接口返回结构异常")
    if interval in TENCENT_MINUTE_KEY:
        rows = block.get(TENCENT_MINUTE_KEY[interval])
    else:
        key = TENCENT_DAILY_KEY.get(interval, "day")
        rows = block.get("qfq" + key) or block.get(key)
    if not isinstance(rows, list) or not rows:
        raise ValueError("腾讯接口无 %s 周期数据" % interval)

    bars = []
    for row in rows:
        if len(row) < 6:
            continue
        bars.append((tencent_time(row[0]), float(row[1]), float(row[2]),
                     float(row[3]), float(row[4]), float(row[5])))
    return build_points(bars)


def parse_eastmoney(payload):
    """东财: data.klines = ['日期,开,收,高,低,量,额,...']。"""
    data = payload.get("data")
    rows = data.get("klines") if isinstance(data, dict) else None
    if not rows:
        raise ValueError("东方财富接口无K线数据(代码可能不存在)")
    bars = []
    for row in rows:
        parts = str(row).split(",")
        if len(parts) < 6:
            continue
        bars.append((normalize_day(parts[0]), float(parts[1]), float(parts[2]),
                     float(parts[3]), float(parts[4]), float(parts[5]),
                     float(parts[6]) if len(parts) > 6 else float(parts[2])))
    return build_points(bars)


def parse_binance(payload):
    """Binance 风格: [[openTime, open, high, low, close, volume, ...], ...]。"""
    rows = payload
    if isinstance(rows, dict):
        rows = rows.get("data", rows.get("result", rows.get("klines")))
    if not isinstance(rows, list) or not rows:
        raise ValueError("Binance接口未返回K线数据")

    bars = []
    for candle in rows:
        if isinstance(candle, dict):
            stamp = candle.get("time", candle.get("timestamp", candle.get("openTime")))
            open_price = candle.get("open")
            high_price = candle.get("high")
            low_price = candle.get("low")
            close_price = candle.get("close")
            volume = candle.get("volume", candle.get("amount", 0))
        else:
            if len(candle) < 6:
                raise ValueError("每根K线至少需要 时间/开/高/低/收/量 六个字段")
            stamp, open_price, high_price, low_price, close_price, volume = candle[:6]
        bars.append((millisecond_time(stamp), float(open_price), float(close_price),
                     float(high_price), float(low_price), float(volume)))
    return build_points(bars)


def parse_okx(payload):
    """OKX: data = [[ts, 开, 高, 低, 收, 量, ...], ...]，且由新到旧。"""
    rows = payload.get("data")
    if not isinstance(rows, list) or not rows:
        raise ValueError("OKX接口未返回K线数据(%s)" % payload.get("msg", ""))
    bars = []
    for row in reversed(rows):
        if len(row) < 6:
            continue
        bars.append((millisecond_time(row[0]), float(row[1]), float(row[4]),
                     float(row[2]), float(row[3]), float(row[5])))
    return build_points(bars)


def parse_sina_inner(payload, code):
    """新浪国内期货(沪金/沪银): [{'d','o','h','l','c','v','p'}, ...]。"""
    if not isinstance(payload, list) or not payload:
        raise ValueError("新浪期货接口无K线数据(%s)" % code)
    bars = []
    for row in payload:
        if not isinstance(row, dict):
            continue
        bars.append((normalize_day(row.get("d", "")), float(row["o"]), float(row["c"]),
                     float(row["h"]), float(row["l"]), float(row.get("v", 0) or 0),
                     float(row.get("p", row["c"]))))
    return build_points(bars)


def parse_sina_global(payload, code):
    """新浪国际期货(伦敦金/纽约金)日线: [{'date','open','high','low','close','volume'}]。"""
    if not isinstance(payload, list) or not payload:
        raise ValueError("新浪国际期货接口无K线数据(%s)" % code)
    bars = []
    for row in payload:
        if not isinstance(row, dict):
            continue
        bars.append((normalize_day(row.get("date", "")), float(row["open"]),
                     float(row["close"]), float(row["high"]), float(row["low"]),
                     float(row.get("volume", 0) or 0)))
    return build_points(bars)


def parse_sina_snapshot(text, code):
    """新浪快照: var hq_str_hf_XAU="最新价,...,时间,昨收,昨结,..,日期,名称";"""
    match = re.search(r'="([^"]*)"', text)
    if not match or not match.group(1):
        raise ValueError("未取到 %s 快照" % code)
    parts = match.group(1).split(",")
    if len(parts) < 13:
        raise ValueError("快照字段不足(%s): %d" % (code, len(parts)))
    price = float(parts[0]) if parts[0] else 0.0
    return "%s %s" % (parts[12], parts[6]), price, (parts[13] if len(parts) > 13 else code)


def parse_sina_minline(payload, code):
    """新浪国际期货分时线: payload['minLine_1d']，返回当日 (时间, 价格) 序列。

    首行为 10 列 [日期, 昨收, 交易所, '', 时间, 价格, '0', '0', 均价, 完整时间]，
    其余行为 6 列 [时间, 价格, '0', '0', 均价, 完整时间]。统一取「完整时间 + 价格」。
    """
    rows = payload.get("minLine_1d")
    if not isinstance(rows, list) or not rows:
        raise ValueError("新浪分时接口无数据(%s)" % code)
    ticks = []
    for row in rows:
        if len(row) >= 10:
            when, price = str(row[9]), row[5]
        elif len(row) >= 6:
            when, price = str(row[5]), row[1]
        else:
            continue
        try:
            ticks.append((when, float(price)))
        except ValueError:
            continue
    if not ticks:
        raise ValueError("新浪分时接口无有效价格(%s)" % code)
    return ticks


def parse_eastmoney_trends(payload, code):
    """东财分时(1分钟)行情: data['trends'] 每行为 "时间,开,收,高,低,量,额,均价"。

    该接口的「开」固定为 0，只有最新价可用，因此返回 (完整时间, 价格) 序列。
    """
    data = (payload or {}).get("data") or {}
    rows = data.get("trends")
    if not isinstance(rows, list) or not rows:
        raise ValueError("东财分时接口无数据(%s)" % code)
    ticks = []
    for row in rows:
        parts = str(row).split(",")
        if len(parts) < 3:
            continue
        try:
            price = float(parts[2])
        except ValueError:
            continue
        if price > 0:
            ticks.append(("%s:00" % parts[0][:16], price))
    if not ticks:
        raise ValueError("东财分时接口无有效价格(%s)" % code)
    return ticks


# ---------------------------------------------------------------------------
# 七、实时快照聚合为分钟K线（国际金没有分钟K线接口）
# ---------------------------------------------------------------------------
class MinuteAggregator:
    """单根K线内部结构: [时间, 开, 高, 低, 收, 量]"""

    def __init__(self, max_bars=1000):
        self.bars = []
        self.index = {}
        self.last_volume = None
        self.max_bars = max_bars

    def push(self, when, price, volume=None):
        if price <= 0:
            return
        bar = self.index.get(when)
        if bar is None:
            bar = [when, price, price, price, price, 0.0]
            self.index[when] = bar
            self.bars.append(bar)
            if len(self.bars) > self.max_bars:
                self.index.pop(self.bars.pop(0)[0], None)
        else:
            if price > bar[2]:
                bar[2] = price
            if price < bar[3]:
                bar[3] = price
            bar[4] = price
        if volume:
            if self.last_volume is not None and volume >= self.last_volume:
                bar[5] += volume - self.last_volume
            self.last_volume = volume

    def points(self):
        return build_points([(bar[0], bar[1], bar[4], bar[2], bar[3], bar[5])
                             for bar in self.bars])


AGGREGATORS = {}


def floor_minute(when, step):
    """把 'YYYY-MM-DD HH:MM:SS' 向下取整到 step 分钟，作为K线的时间桶。"""
    try:
        stamp = datetime.strptime(when[:19], "%Y-%m-%d %H:%M:%S")
    except ValueError:
        return when[:16]
    return stamp.replace(minute=(stamp.minute // step) * step,
                         second=0).strftime("%Y-%m-%d %H:%M:%S")


def aggregate_snapshot(source, ticks, step, name, snapshot_code):
    """把分时/快照价格序列按 step 分钟聚合成K线（部分品种没有分钟K线接口）。

    分时线一次就能提供当日（或近几日）全部分钟价格，因此 5m/15m/30m/60m 周期可立即
    得到带真实高低点的K线；1m 周期为每分钟单价的平价K线。
    """
    aggregator = AGGREGATORS.get(source)
    if aggregator is None:
        aggregator = AGGREGATORS[source] = MinuteAggregator()
    for when, price in ticks:
        aggregator.push(floor_minute(when, step), price)
    if not aggregator.bars:
        raise ValueError("%s 暂无有效价格(%s)" % (name, snapshot_code))
    return aggregator.points()


# ---------------------------------------------------------------------------
# 八、各数据源加载器：统一接收 Query，由 SOURCE_LOADERS 分发
# ---------------------------------------------------------------------------
Query = namedtuple("Query", "source code interval limit timeout start end")


def _load_tencent(query):
    conf = SOURCES[query.source]
    headers = {"User-Agent": USER_AGENT, "Referer": TENCENT_REFERER}
    if query.interval in TENCENT_MINUTE_KEY:
        params = {"param": "%s,%s,,%d"
                  % (query.code, TENCENT_MINUTE_KEY[query.interval], query.limit)}
        urls = conf["minute_url"]
    else:
        # 日线及以上支持 code,period,start,end,count 的原生历史区间查询
        key = TENCENT_DAILY_KEY.get(query.interval, "day")
        params = {"param": "%s,%s,%s,%s,%d,qfq"
                  % (query.code, key, query.start or "", query.end or "", query.limit)}
        urls = conf["kline_url"]
    return fetch_first(urls, params, query.timeout, headers,
                       lambda payload: parse_tencent(payload, query.code, query.interval))


def _load_eastmoney(query):
    params = {"secid": query.code, "fields1": "f1,f2,f3,f4,f5,f6",
              "fields2": "f51,f52,f53,f54,f55,f56,f57",
              "klt": EASTMONEY_KLT.get(query.interval, 101), "fqt": 1,
              "beg": (query.start or "19900101").replace("-", ""),
              "end": (query.end or "20500101").replace("-", ""),
              "lmt": query.limit}
    return fetch_first(SOURCES[query.source]["kline_url"], params, query.timeout,
                       default_headers(), parse_eastmoney)


def _load_binance(query):
    return load_binance_points(SOURCES[query.source]["kline_url"], query)


def _load_okx(query):
    params = {"instId": query.code, "bar": OKX_BAR.get(query.interval, "1m"),
              "limit": query.limit}
    if query.start:
        params["before"] = to_timestamp_ms(query.start)       # 取该时刻之后(更新)的数据
    if query.end:
        params["after"] = to_timestamp_ms(query.end, True)    # 取该时刻之前(更旧)的数据
    return fetch_first(SOURCES[query.source]["kline_url"], params, query.timeout,
                       default_headers(), parse_okx)


def _load_gold(query):
    """沪金/沪银等国内期货：日线返回全量历史，分钟线返回近几日，均为真实OHLC。"""
    if query.interval == "1d":
        url = SINA_INNER_URL % (query.code, "getDailyKLine")
        params = {"symbol": query.code}
    else:
        url = SINA_INNER_URL % (query.code, "getFewMinLine")
        params = {"symbol": query.code, "type": MINUTE_STEP[query.interval]}
    payload = parse_jsonp(fetch_text(url, params, query.timeout, headers=sina_headers()))
    return parse_sina_inner(payload, query.code)


def _load_sina_futures(query):
    """新浪国际期货（黄金/原油/天然气）：日线为真实OHLC；分钟线用当日分时线 + 快照聚合。"""
    conf = SINA_FUTURES[query.source]
    code = query.code or conf["code"]
    if query.interval == "1d":
        url = SINA_GLOBAL_URL % (code, "getGlobalFuturesDailyKLine")
        payload = parse_jsonp(fetch_text(url, {"symbol": code}, query.timeout,
                                         headers=sina_headers()))
        return parse_sina_global(payload, code)

    # 当日分时线：一次拿到全天每分钟价格，无需从启动开始慢慢累积
    url = SINA_GLOBAL_URL % (code, "getGlobalFuturesMinLine")
    payload = parse_jsonp(fetch_text(url, {"symbol": code}, query.timeout,
                                     headers=sina_headers()))
    ticks = parse_sina_minline(payload, code)

    # 再补一笔最新快照，让最后一根K线跟到最新价
    snapshot_code = conf["snapshot"]
    text = fetch_text(SINA_SNAPSHOT_URL % snapshot_code, None, query.timeout,
                      encoding="gbk", headers=sina_headers())
    when, price, name = parse_sina_snapshot(text, snapshot_code)
    ticks.append((when, price))

    return aggregate_snapshot(query.source, ticks, MINUTE_STEP[query.interval],
                              name, snapshot_code)


def _load_usd_index(query):
    """美元指数：用东财分时（最多5个交易日）聚合出分钟K线。

    东财K线接口对 100.UDI 没有数据、新浪外汇也没有历史日线，因此不支持 1d，
    请求日线时由 SUPPORTED_INTERVALS 在命令行阶段直接报错，而不是静默降级。
    """
    conf = SOURCES[query.source]
    secid = conf["trends_secid"]
    days = 1 if query.interval == "1m" else 5     # 1分钟只看当日，其它周期多取几天凑根数
    params = {"secid": secid, "fields1": "f1,f2,f3,f4",
              "fields2": "f51,f52,f53,f54,f55,f56", "iscr": 0, "ndays": days}
    payload = fetch_json(EASTMONEY_TRENDS_URL, params, query.timeout,
                         headers=default_headers())
    ticks = parse_eastmoney_trends(payload, secid)
    return aggregate_snapshot(query.source, ticks, MINUTE_STEP[query.interval],
                              "美元指数", secid)


SOURCE_LOADERS = {
    "tencent": _load_tencent,
    "eastmoney": _load_eastmoney,
    "binance": _load_binance,
    "okx": _load_okx,
    "gold": _load_gold,
    "usd": _load_usd_index,
}
# 新浪国际期货共用同一份加载器（xau/gc/crude/brent/ng）
SOURCE_LOADERS.update({name: _load_sina_futures for name in SINA_FUTURES})


def load_binance_points(urls, query):
    """Binance 风格接口：单次最多1000根；带历史区间时按 startTime 分页取全。"""
    max_rows = SINGLE_REQUEST_MAX["binance"]
    limit = min(query.limit, max_rows)

    def build_params(cursor=None, end_ts=None):
        params = {"interval": query.interval, "limit": limit}
        if query.code:
            params["symbol"] = query.code
        if cursor is not None:
            params["startTime"] = cursor
        if end_ts is not None:
            params["endTime"] = end_ts
        return params

    if not query.start and not query.end:
        return fetch_first(urls, build_params(), query.timeout,
                           default_headers(), parse_binance)

    rows = []
    last_error = None
    cursor = to_timestamp_ms(query.start) if query.start else None
    end_ts = to_timestamp_ms(query.end, True) if query.end else None
    for _ in range(50):                     # 最多50页，异常时不会死循环
        batch = None
        for url in urls:
            try:
                batch = fetch_json(url, build_params(cursor, end_ts), query.timeout,
                                   headers=default_headers())
                break
            except (OSError, ValueError, json.JSONDecodeError) as error:
                last_error = error
        if not isinstance(batch, list) or not batch:
            break
        rows.extend(batch)
        if len(batch) < max_rows:
            break
        next_cursor = int(batch[-1][0]) + 1
        if end_ts is not None and next_cursor >= end_ts:
            break
        cursor = next_cursor
    if not rows:
        raise last_error if last_error else ValueError("Binance接口未返回K线数据")
    rows.sort(key=lambda candle: candle[0])
    return parse_binance(rows)


def load_source_points(source, symbol, interval, limit=120, timeout=10, start=None, end=None):
    """按数据源拉取K线。start/end 为 'YYYY-MM-DD'，limit<=0 表示不限条数。"""
    loader = SOURCE_LOADERS.get(source)
    if loader is None:
        raise ValueError("未知数据源: %s" % source)
    query = Query(source,
                  normalize_symbol(source, symbol),
                  validate_interval(source, normalize_interval(interval)),
                  request_limit(limit, SINGLE_REQUEST_MAX.get(source, 0)),
                  timeout, start, end)
    return shape_points(drop_future_points(loader(query)), start, end, limit)


def resolve_points(source, symbol, interval, limit=120, timeout=10, start=None, end=None):
    """按 --source 取数；auto 时逐个数据源容错回退。返回 (源名, 实际代码, points)。"""
    names = candidate_sources(symbol) if source == "auto" else [source]
    errors = []
    for name in names:
        try:
            code = normalize_symbol(name, symbol)
            points = load_source_points(name, symbol, interval, limit, timeout, start, end)
            return name, code, points
        except (OSError, ValueError, json.JSONDecodeError, KeyError) as error:
            # 周期不支持属于参数问题，auto 也不应靠回退把它掩盖掉
            if source != "auto" or isinstance(error, UnsupportedInterval):
                raise
            errors.append("%s: %s" % (name, error))
    raise ValueError("所有数据源均不可用 -> " + " | ".join(errors))


def load_api_points(url, symbol=None, interval="1m", limit=120, timeout=10,
                    start=None, end=None):
    """兼容旧用法：从自定义 REST 地址(Binance 风格 klines)读取，支持历史区间。"""
    query = Query("binance", symbol or "", normalize_interval(interval),
                  request_limit(limit, SINGLE_REQUEST_MAX["binance"]),
                  timeout, start, end)
    return shape_points(load_binance_points([url], query), start, end, limit)


def load_file_points(path):
    """读取本地 .DAT K线文件（GB2312，前两行为表头）。"""
    rows = []
    with open(path, "r", encoding=FILE_ENCODING) as handle:
        for count, line in enumerate(handle):
            fields = line.split("\t") if count > 1 else line.split(" ")
            if count <= 1:
                print("stock: " + ", ".join(field.strip() for field in fields[:3]))
                continue
            if len(fields) < 7:
                continue
            rows.append((fields[0], float(fields[1]), float(fields[4]),
                         float(fields[2]), float(fields[3]),
                         int(fields[5]), float(fields[6])))
    return build_points(rows)


# ---------------------------------------------------------------------------
# 九、坐标与刻度
# ---------------------------------------------------------------------------
def _parse_stamp(text):
    """把各种写法的日期字符串解析成 datetime，失败返回 None。"""
    stamp = str(text)[:19].replace("/", "-").replace(".", "-")
    for pattern, length in (("%Y-%m-%d %H:%M:%S", 19),
                            ("%Y-%m-%d %H:%M", 16),
                            ("%Y-%m-%d", 10)):
        if len(stamp) >= length:
            try:
                return datetime.strptime(stamp[:length], pattern)
            except ValueError:
                continue
    return None


def time_label_format(date):
    """按数据跨度选择时间刻度格式。

    先区分日线/日内：日线一律用日期格式（避免出现无意义的 00:00）；
    日内数据同日→时分，跨日→月-日 时:分，更长→只标月-日。
    """
    samples = [_parse_stamp(date[index]) for index in (0, len(date) // 2, len(date) - 1)]
    samples = [stamp for stamp in samples if stamp is not None]
    if not samples:
        return "%Y-%m-%d"
    first, last = samples[0], samples[-1]
    days = (last - first).days
    same_year = first.year == last.year
    if not any(stamp.hour or stamp.minute or stamp.second for stamp in samples):
        if days <= 120 or (days <= 400 and same_year):
            return "%m-%d"
        if days <= 1200:
            return "%Y-%m"
        return "%Y"
    if first.date() == last.date():
        return "%H:%M"
    if days <= 3:
        return "%m-%d %H:%M"
    # 跨年的日内数据只用月-日会重复，故退回年-月
    return "%m-%d" if same_year else "%Y-%m"


def time_ticks(date, max_ticks=8):
    """返回 (刻度位置, 刻度标签)：只在标签内容变化的边界取点，避免重复与过长标签。"""
    fmt = time_label_format(date)
    labels = []
    for when in date:
        stamp = _parse_stamp(when)
        labels.append(stamp.strftime(fmt) if stamp else str(when)[:10])
    picks = []
    for index, label in enumerate(labels):
        if not picks or label != labels[picks[-1]]:
            picks.append(index)
    if len(picks) > max_ticks:
        stride = int(np.ceil(len(picks) / float(max_ticks)))
        reduced = picks[::stride]
        if reduced[-1] != picks[-1]:
            reduced.append(picks[-1])
        picks = reduced
    return picks, [labels[index] for index in picks]


def price_decimals(low, high):
    """按价格区间大小决定需要的小数位，避免价位刻度精度失真。

    阈值按「区间跨度」而非绝对价格选取：美元指数这类 ~100 且日内波动 1 点左右的品种
    取 2 位小数即可，不必铺满 3 位。
    """
    span = abs(float(high) - float(low))
    for limit, digits in ((500, 0), (20, 1), (1, 2), (0.05, 3)):
        if span >= limit:
            return digits
    return 4


def compact_number(value, decimals=0):
    """大数字压缩显示(亿/万)，避免成交量轴出现 1e6 这类偏移量。"""
    value = float(value)
    if abs(value) >= 1e8:
        return "%.2f亿" % (value / 1e8)
    if abs(value) >= 1e4:
        scaled = value / 1e4
        return ("%.2f万" % scaled) if abs(scaled) < 100 else ("%.0f万" % scaled)
    return ("%%.%df" % decimals) % value


def apply_time_axis(axis, date, max_ticks=8):
    """给X轴套上智能时间刻度，标签过长时自动倾斜。

    刻度标签用 FixedLocator + FuncFormatter，而不是 set_xticklabels：
    后者会装上 FixedFormatter，而它的 format_data_short() 恒返回空串，
    会让鼠标悬停时右下角的横坐标显示为空（format_coord -> format_xdata）。
    另外把悬停文本挂到 Axes.fmt_xdata 上，这样悬停显示该根K线的完整时间，
    而刻度标签仍保持精简。
    """
    positions, labels = time_ticks(date, max_ticks)
    fmt = time_label_format(date)

    def bar_index(value):
        index = int(round(value))
        return index if 0 <= index < len(date) else None

    def tick_label(value, _pos=None):
        index = bar_index(value)
        if index is None:
            return ""
        stamp = _parse_stamp(date[index])
        return stamp.strftime(fmt) if stamp is not None else str(date[index])[:10]

    def hover_label(value):
        index = bar_index(value)
        return str(date[index]) if index is not None else "%.0f" % value

    tilted = any(" " in label for label in labels)
    axis.xaxis.set_major_locator(FixedLocator(positions))
    axis.xaxis.set_major_formatter(FuncFormatter(tick_label))
    axis.fmt_xdata = hover_label
    axis.tick_params(axis="x", length=3, labelsize=8,
                     labelrotation=30 if tilted else 0)
    plt.setp(axis.get_xticklabels(), ha="right" if tilted else "center")


def _clean_spines(axis):
    """统一坐标轴观感：去掉上/右边框，只保留横向网格。"""
    for side in ("top", "right"):
        axis.spines[side].set_visible(False)
    axis.grid(True, axis="y", color=STYLE["grid"], linewidth=0.6)
    axis.grid(False, axis="x")
    axis.set_axisbelow(True)


def style_price_axis(axis, low, high):
    """价格轴：自适应小数位 + 等间隔刻度。"""
    axis.yaxis.set_major_locator(MaxNLocator(nbins=7, prune="both"))
    axis.yaxis.set_major_formatter(FuncFormatter(
        lambda value, _pos: "%.*f" % (price_decimals(low, high), value)))
    axis.tick_params(axis="y", labelsize=8, pad=3)
    _clean_spines(axis)


def style_panel_axis(axis, label, formatter=None, decimals=2):
    """底部副图坐标轴：刻度精简 + 轴内标题（替代竖排 ylabel）。"""
    axis.yaxis.set_major_locator(MaxNLocator(nbins=3, prune="both"))
    if formatter is None:
        formatter = lambda value, _pos: "%.*f" % (decimals, value)
    axis.yaxis.set_major_formatter(FuncFormatter(formatter))
    axis.tick_params(axis="y", labelsize=8, pad=3)
    _clean_spines(axis)
    axis.text(0.005, 0.97, label, transform=axis.transAxes, ha="left", va="top",
              fontsize=8, color=STYLE["muted"],
              bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="none", alpha=0.72))


# ---------------------------------------------------------------------------
# 十、绘图
# ---------------------------------------------------------------------------
def _on_click(event):
    """单击底部副图，在 成交量 -> MACD -> RSI 之间循环切换。"""
    figure = event.canvas.figure
    store = getattr(figure, "_matkline_hover", None)
    if not store or event.button != 1 or event.inaxes is not store["axes"][1]:
        return
    figure._matkline_panel = (getattr(figure, "_matkline_panel", 0) + 1) % len(PANEL_MODES)
    redraw = getattr(figure, "_matkline_redraw", None)
    if redraw:
        redraw()
    event.canvas.draw_idle()


def ema(values, span):
    """指数移动平均。"""
    alpha = 2.0 / (span + 1.0)
    out = np.empty(len(values), dtype=float)
    if len(values) == 0:
        return out
    out[0] = values[0]
    for index in range(1, len(values)):
        out[index] = alpha * values[index] + (1.0 - alpha) * out[index - 1]
    return out


def macd(values, fast=12, slow=26, signal=9):
    """MACD：返回 (DIF, DEA, 柱)，柱 = 2*(DIF-DEA)，与行情软件口径一致。"""
    dif = ema(values, fast) - ema(values, slow)
    dea = ema(dif, signal)
    return dif, dea, 2.0 * (dif - dea)


def rsi(values, period=14):
    """RSI（Wilder 平滑）。数据不足 period+1 根时返回全 NaN。"""
    out = np.full(len(values), np.nan)
    if len(values) <= period:
        return out
    delta = np.diff(values)
    gains = np.where(delta > 0, delta, 0.0)
    losses = np.where(delta < 0, -delta, 0.0)
    avg_gain, avg_loss = gains[:period].mean(), losses[:period].mean()
    for index in range(period, len(values)):
        if index > period:
            avg_gain = (avg_gain * (period - 1) + gains[index - 1]) / period
            avg_loss = (avg_loss * (period - 1) + losses[index - 1]) / period
        out[index] = 100.0 if avg_loss == 0 else 100.0 - 100.0 / (1.0 + avg_gain / avg_loss)
    return out


def kdj(high, low, close, period=9, k_period=3, d_period=3):
    """KDJ：RSV 取 period 日内最高最低，K/D 各做一次平滑，J = 3K - 2D。"""
    count = len(close)
    k_out = np.full(count, np.nan)
    d_out = np.full(count, np.nan)
    j_out = np.full(count, np.nan)
    k_prev = d_prev = 50.0                     # 与行情软件一致：K/D 从 50 起算
    for index in range(count):
        start = max(0, index - period + 1)
        highest = float(high[start:index + 1].max())
        lowest = float(low[start:index + 1].min())
        rsv = 50.0 if highest == lowest else (close[index] - lowest) / (highest - lowest) * 100.0
        k_prev = (k_prev * (k_period - 1) + rsv) / k_period
        d_prev = (d_prev * (d_period - 1) + k_prev) / d_period
        k_out[index], d_out[index] = k_prev, d_prev
        j_out[index] = 3.0 * k_prev - 2.0 * d_prev
    return k_out, d_out, j_out


def panel_label(mode, params):
    """副图标题：把当前指标参数一并显示，方便确认 --macd/--rsi/--kdj 已生效。"""
    return PANEL_LABEL_FMT[mode] % tuple(params.get(mode, ()))


def _draw_panel(axis, x, mode, volume, colors, close_arr, high_arr, low_arr, params):
    """画底部副图，返回该面板的纵轴数值格式化函数（None 表示沿用价格精度）。

    指标数据不足时回退到成交量面板；无成交量的数据源（如国际金）给出文字提示，
    而不是画一个空白面板。
    """
    if mode == "macd":
        dif, dea, hist = macd(close_arr, *params["macd"])
        axis.bar(x, hist, width=0.6, color=np.where(hist >= 0, UP_COLOR, DOWN_COLOR))
        axis.plot(x, dif, color="orange", linewidth=0.9, label="DIF")
        axis.plot(x, dea, color="royalblue", linewidth=0.9, label="DEA")
        axis.axhline(0, color="0.6", linewidth=0.6)
        axis.legend(loc="upper right", fontsize=7, ncol=2, handlelength=1.2,
                    frameon=True, framealpha=0.85, edgecolor="none")
        return None
    if mode == "rsi" and len(close_arr) > params["rsi"][0]:
        axis.plot(x, rsi(close_arr, *params["rsi"]), color="purple", linewidth=0.9)
        for level in (30, 50, 70):
            axis.axhline(level, color="0.75", linewidth=0.6, linestyle="--")
        axis.set_ylim(0, 100)
        axis.set_autoscaley_on(False)
        return lambda value, _pos: "%.0f" % value
    if mode == "kdj":
        k_line, d_line, j_line = kdj(high_arr, low_arr, close_arr, *params["kdj"])
        axis.plot(x, k_line, color="royalblue", linewidth=0.9, label="K")
        axis.plot(x, d_line, color="orange", linewidth=0.9, label="D")
        axis.plot(x, j_line, color="purple", linewidth=0.9, label="J")
        for level in (20, 50, 80):
            axis.axhline(level, color="0.75", linewidth=0.6, linestyle="--")
        axis.set_ylim(-20, 120)                # J 常冲出 0~100，上下留余量
        axis.set_autoscaley_on(False)
        axis.legend(loc="upper right", fontsize=7, ncol=3, handlelength=1.2,
                    frameon=True, framealpha=0.85, edgecolor="none")
        return lambda value, _pos: "%.0f" % value
    if not np.any(volume > 0):
        axis.text(0.5, 0.5, "该数据源无成交量", transform=axis.transAxes,
                  ha="center", va="center", fontsize=9, color="0.55")
        axis.set_yticks([])
        axis.set_ylim(0, 1)
        axis.set_autoscaley_on(False)
        return None
    axis.bar(x, volume, width=0.6, color=colors)
    axis.set_ylim(0, float(volume.max()) * 1.15)
    axis.set_autoscaley_on(False)
    return lambda value, _pos: compact_number(value)


def _swatch(text, color=None, size=8):
    """信息框里的一小段文字（可单独上色）。"""
    return TextArea(text, textprops=dict(fontsize=size,
                                         color=color or STYLE["text"]))


def _pair(label, value, color=None):
    """「标签 + 数值」紧贴成一组，数值可按颜色高亮。"""
    return HPacker(children=[_swatch(label + " ", STYLE["muted"]),
                             _swatch(value, color)],
                   align="baseline", pad=0, sep=1)


def _bar_box(store, index):
    """拼出悬停信息框：时间 / 开收 / 高低 / 量·涨跌。

    收与涨跌按相对前一根收盘（首根退化用开盘）红涨绿跌；高恒红、低恒绿，
    与行情软件的配色习惯一致。无成交量的数据源不显示成交量一栏。
    """
    fmt = "%%.%df" % store["digits"]
    open_val, close_val = store["open"][index], store["close"][index]
    high_val, low_val = store["high"][index], store["low"][index]
    base = store["close"][index - 1] if index > 0 else open_val
    change = (close_val - base) / base * 100.0 if base else 0.0
    tone = UP_COLOR if change >= 0 else DOWN_COLOR
    rows = [_swatch(str(store["date"][index]), STYLE["muted"])]
    rows.append(HPacker(children=[_pair("开", fmt % open_val),
                                  _pair("收", fmt % close_val, tone)],
                        align="baseline", pad=0, sep=10))
    rows.append(HPacker(children=[_pair("高", fmt % high_val, UP_COLOR),
                                  _pair("低", fmt % low_val, DOWN_COLOR)],
                        align="baseline", pad=0, sep=10))
    tail = [_pair("涨跌", "%+.2f%%" % change, tone)]
    if store["has_volume"]:
        tail.insert(0, _pair("量", compact_number(store["volume"][index], 2)))
    rows.append(HPacker(children=tail, align="baseline", pad=0, sep=10))
    return VPacker(children=rows, align="left", pad=0, sep=2)


def _place_hover(store, index):
    """把信息框放到该根K线旁（光标在右半区时改放左侧，避免超出画布）。"""
    axis = store["axes"][0]
    if store["box"] is not None:
        store["box"].remove()
    on_left = index > store["count"] * 0.62
    box = AnnotationBbox(
        _bar_box(store, index), (index, float(store["close"][index])),
        xycoords="data", boxcoords="offset points",
        xybox=(-12, 12) if on_left else (12, 12),
        box_alignment=(1.0, 0.0) if on_left else (0.0, 0.0),
        frameon=True, pad=0,
        bboxprops=dict(boxstyle="round,pad=0.45", fc="white", ec="0.6", alpha=0.92))
    box.set_zorder(10)
    axis.add_artist(box)
    store["box"] = box


def _hover_artists(axis):
    """创建悬停用的高亮竖线；信息框在悬停时按需生成（多色排版需要重建）。"""
    return {
        "vline": axis.axvline(0, color="0.45", linewidth=0.7, linestyle="--",
                              visible=False, zorder=6),
        "box": None,
    }


def _on_hover(event):
    """鼠标悬停：高亮该根K线，并在旁边显示其开高低收量。

    只在「换到另一根K线」时重绘：横向不做跟随，避免每次鼠标移动都整图重画。
    """
    figure = event.canvas.figure
    store = getattr(figure, "_matkline_hover", None)
    if not store:
        return
    index = None
    if event.inaxes in store["axes"] and event.xdata is not None:
        candidate = int(round(event.xdata))
        if 0 <= candidate < store["count"]:
            index = candidate
    if index == store["last"]:            # 同一根K线内移动不重绘
        return
    store["last"] = index
    visible = index is not None
    store["vline"].set_visible(visible)
    if visible:
        store["vline"].set_xdata([index, index])
        _place_hover(store, index)
    elif store["box"] is not None:
        store["box"].remove()
        store["box"] = None
    event.canvas.draw_idle()


def _install_events(figure):
    """给 figure 挂上悬停/单击回调（同一个 figure 只挂一次）。"""
    if getattr(figure, "_matkline_events_ready", False):
        return
    figure._matkline_events_ready = True
    figure.canvas.mpl_connect("motion_notify_event", _on_hover)
    figure.canvas.mpl_connect("button_press_event", _on_click)


def plot_points(points, title):
    """折线模式：开/收/高/低四条线（配色与蜡烛图保持一致）。"""
    if not points[0]:
        return
    figure = plt.gcf()
    figure.clf()
    figure._matkline_hover = None         # 折线模式不提供K线悬停提示
    axis = figure.add_subplot(111)
    x = np.arange(len(points[0]))
    for values, label, color, style in ((points[2], "开", MA_COLORS[0][1], "-"),
                                        (points[3], "收", STYLE["text"], "-"),
                                        (points[4], "高", UP_COLOR, "--"),
                                        (points[5], "低", DOWN_COLOR, "--")):
        axis.plot(x, values, label=label, color=color, linestyle=style, linewidth=1.0)
    axis.set_title(title, loc="left", pad=12)
    apply_time_axis(axis, points[0])
    style_price_axis(axis, min(points[5]), max(points[4]))
    axis.legend(loc="upper left", ncol=4, columnspacing=1.0, handlelength=1.4,
                frameon=True, framealpha=0.85, edgecolor="none")
    figure.subplots_adjust(left=0.065, right=0.975, top=0.9, bottom=0.12)


def plot_candles(points, title, bars=None, params=None):
    """蜡烛图模式：红涨绿跌 + 影线 + MA5/10/20 + 可切换副图。

    bars=None 表示画出全部数据（取多少根由 --limit 控制）；
    params 为指标参数（缺省用 DEFAULT_PARAMS），由调用方一路传进来。
    """
    params = params or DEFAULT_PARAMS
    total = len(points[0])
    if total == 0:
        return
    first = max(0, total - bars) if bars else 0
    date = points[0][first:]
    volume = np.array(points[1][first:], dtype=float)
    open_arr = np.array(points[2][first:], dtype=float)
    close_arr = np.array(points[3][first:], dtype=float)
    high_arr = np.array(points[4][first:], dtype=float)
    low_arr = np.array(points[5][first:], dtype=float)
    x = np.arange(len(date))
    rising = close_arr >= open_arr
    colors = np.where(rising, UP_COLOR, DOWN_COLOR)

    figure = plt.gcf()
    figure.clf()
    # 没有成交量的数据源（如国际金）默认展示 MACD，而不是一片空面板
    if not hasattr(figure, "_matkline_panel"):
        figure._matkline_panel = 0 if np.any(volume > 0) else 1
    panel_mode = PANEL_MODES[figure._matkline_panel % len(PANEL_MODES)]

    axis, panel_axis = figure.subplots(
        2, 1, sharex=True,
        gridspec_kw={"height_ratios": [3, 1], "hspace": 0.08})
    hover = _hover_artists(axis)      # 先建好并隐藏，避免影响下面的坐标范围

    # 影线 + 实体：K线越密实体越宽，避免整片糊在一起
    count = len(x)
    body = 0.62 if count <= 240 else (0.76 if count <= 600 else 0.9)
    wick = 0.85 if count <= 600 else 0.6
    half = body / 2.0
    axis.vlines(x, low_arr, high_arr, color=colors, linewidth=wick)
    for xi, oi, ci, up in zip(x, open_arr, close_arr, rising):
        if up:
            axis.add_patch(Rectangle((xi - half, oi), body, max(ci - oi, 1e-9),
                                     facecolor=UP_COLOR, edgecolor=UP_COLOR, linewidth=0))
        else:
            axis.add_patch(Rectangle((xi - half, ci), body, oi - ci,
                                     facecolor=DOWN_COLOR, edgecolor=DOWN_COLOR, linewidth=0))

    # 均线
    for window, color in MA_COLORS:
        if len(close_arr) >= window:
            ma = np.convolve(close_arr, np.ones(window) / window, mode="valid")
            axis.plot(x[window - 1:], ma, color=color, linewidth=1.0,
                      label="MA%d" % window)

    low_min = float(low_arr.min())
    high_max = float(high_arr.max())
    pad = (high_max - low_min) * 0.06 or max(abs(high_max) * 0.01, 0.5)
    decimals = price_decimals(low_min, high_max)     # 悬停提示里的价格精度
    axis.set_xlim(-1, len(x))
    axis.set_ylim(low_min - pad, high_max + pad)
    axis.set_autoscalex_on(False)     # 固定坐标范围，避免悬停十字光标改变缩放
    axis.set_autoscaley_on(False)
    style_price_axis(axis, low_min, high_max)

    # 标题靠左，右上角放最新价与涨跌幅（红涨绿跌），并画出最新价虚线
    last_close = float(close_arr[-1])
    base_close = float(close_arr[-2]) if len(close_arr) > 1 else float(open_arr[-1])
    change = (last_close - base_close) / base_close * 100.0 if base_close else 0.0
    tone = UP_COLOR if change >= 0 else DOWN_COLOR
    axis.set_title(title, loc="left", pad=12)
    axis.text(1.0, 1.012, "最新 %.*f   %+.2f%%" % (decimals, last_close, change),
              transform=axis.transAxes, ha="right", va="bottom",
              fontsize=9.5, color=tone)
    axis.axhline(last_close, color=tone, linewidth=0.7, linestyle="--",
                 alpha=0.65, zorder=2)
    if len(close_arr) >= 5:
        axis.legend(loc="upper left", ncol=3, columnspacing=1.0, handlelength=1.4,
                    frameon=True, framealpha=0.85, edgecolor="none")
    axis.tick_params(labelbottom=False)

    panel_formatter = _draw_panel(
        panel_axis, x, panel_mode, volume, colors, close_arr, high_arr, low_arr, params)
    style_panel_axis(panel_axis, panel_label(panel_mode, params), panel_formatter, decimals)
    panel_axis.text(0.995, 0.06, "单击切换副图", transform=panel_axis.transAxes,
                    ha="right", va="bottom", fontsize=7.5, color=STYLE["hint"],
                    bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="none", alpha=0.72))
    apply_time_axis(panel_axis, date)
    axis.fmt_xdata = panel_axis.fmt_xdata           # 价格副图同样按时间显示横坐标

    # 悬停提示数据（每次重绘都换成新一组图元）
    hover.update({
        "axes": (axis, panel_axis), "date": date, "open": open_arr,
        "close": close_arr, "high": high_arr, "low": low_arr, "volume": volume,
        "count": len(date), "digits": decimals, "last": None,
        "has_volume": bool(np.any(volume > 0)),
    })
    figure._matkline_hover = hover
    figure._matkline_redraw = lambda: plot_candles(points, title, bars, params)
    _install_events(figure)

    figure.subplots_adjust(left=0.065, right=0.975, top=0.9, bottom=0.125)


# ---------------------------------------------------------------------------
# 十一、实时刷新
# ---------------------------------------------------------------------------
def _refresh_wait(figure, interval):
    """刷新画面并等待 interval 秒，但不会把窗口提到最前。

    不能用 plt.pause()：它内部每帧都会调用 show(block=False)，而后端的
    FigureManager.show() 会 deiconify()+lift()/raise_()，导致每刷新一次就抢一次焦点。
    这里只在首帧开一次窗，之后仅重绘并处理GUI事件（按小片休眠，保证期间可交互）。
    """
    canvas = figure.canvas
    canvas.draw()                                       # 先重绘再开窗，避免首帧白屏
    if not getattr(figure, "_matkline_shown", False):
        figure._matkline_shown = True
        if plt.get_backend().lower() not in NON_GUI_BACKENDS:
            plt.show(block=False)                       # 只开这一次窗，之后不再 show
    deadline = time.monotonic() + max(interval, 0.0)
    while True:
        canvas.flush_events()                           # 只处理GUI事件，不再触发 show
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break
        time.sleep(min(remaining, 0.05))


def _now_text():
    """打印前缀：时间。"""
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")


def set_window_title(figure, text):
    """把标题同步到窗口标题栏；没有 manager 的后端静默跳过。"""
    manager = getattr(figure.canvas, "manager", None)
    if manager is None:
        return
    try:
        manager.set_window_title("K线 - %s" % text)
    except (AttributeError, NotImplementedError):
        pass


def stream_api(loader, refresh=5.0, chart="candle", csv_path=None, params=None):
    """轮询数据源并刷新图表，直到窗口关闭。loader() 返回 (标题, points)。

    指定 csv_path 时每帧把最新K线覆盖写入该文件（导出失败只提示，不影响刷新）。
    """
    if chart == "candle":
        render = partial(plot_candles, bars=None, params=params)
    else:
        render = plot_points
    figure = plt.figure(figsize=(12, 6))
    if chart == "candle":
        print("提示：" + PANEL_HINT)
    while plt.fignum_exists(figure.number):
        try:
            title, points = loader()
            render(points, title)
            set_window_title(figure, title)
            tail = ""
            if csv_path:
                try:
                    export_csv(points, csv_path)
                    tail = "  csv=%s" % csv_path
                except OSError as error:
                    tail = "  导出CSV失败: %s" % error
            print("%s  %s bars, last=%s%s"
                  % (_now_text(), len(points[0]), points[3][-1], tail))
        except (OSError, ValueError, json.JSONDecodeError, KeyError) as error:
            print("%s  刷新失败: %s" % (_now_text(), error), file=sys.stderr)
        _refresh_wait(figure, refresh)


# ---------------------------------------------------------------------------
# 十二、命令行
# ---------------------------------------------------------------------------
EXAMPLES = """\
常用示例:
  python3 matkline.py --live --source gold --interval 1m        # 沪金实时分钟K线
  python3 matkline.py --live --source xau  --interval 5m        # 国际金价(伦敦金现货)
  python3 matkline.py --live --source gc   --interval 1h        # 国际金价(纽约金COMEX)
  python3 matkline.py --source xau --interval 1d --limit 250    # 国际金日线
  python3 matkline.py --history --symbol 600519 --interval 1d --start 2024-01-01 --end 2024-06-30
  python3 matkline.py --source tencent --symbol sh600519 --interval 1d --limit 250
  python3 matkline.py --source auto --symbol 黄金 --interval 5m  # 按标的自动选源
  python3 matkline.py --source crude --interval 1d --limit 250   # WTI原油日线
  python3 matkline.py --live --source brent --interval 1h        # 布伦特原油实时
  python3 matkline.py --source usd --interval 30m --limit 200    # 美元指数30分钟
  python3 matkline.py --source auto --symbol 美元指数 --interval 1h  # 中文标的自动选源
  python3 matkline.py --start 2007-01-01 --end 2007-12-31       # 本地示例数据
  python3 matkline.py --history --symbol 600519 --csv 600519.csv  # 导出CSV(utf-8-sig)
  python3 matkline.py --live --source xau --macd 6,13,5 --rsi 6 --kdj 9,3,3  # 自定义指标参数
数据源: tencent(A股) eastmoney(东财) gold(沪金/沪银) xau/gc(国际金) crude(美原油)
        brent(布伦特) ng(天然气) usd(美元指数) binance/okx(加密货币)
国际盘: xau/gc/crude/brent/ng 支持 1m/5m/15m/30m/60m/1h/1d；usd(美元指数) 支持 1m~1h
交互: 悬停蜡烛图看该根K线详情；单击底部副图在 成交量/MACD/RSI/KDJ 之间切换
"""


def build_parser():
    parser = argparse.ArgumentParser(
        description="本地/实时K线图工具：支持历史K线查询、实时刷新与蜡烛图绘制",
        epilog=EXAMPLES,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("file", nargs="?", default=DEFAULT_FILE,
                        help="本地K线文件(离线模式，默认指向仓库内示例数据)")
    parser.add_argument("--live", action="store_true",
                        help="持续轮询刷新K线（配合 --start/--end 则只在区间内刷新）")
    parser.add_argument("--history", action="store_true",
                        help="历史K线查询：联网只取一次并绘图，数据源仍由 --source 决定")
    parser.add_argument("--source", default="auto", choices=["auto"] + list(SOURCES),
                        help="行情数据源；auto 会按标的自动选择并容错回退")
    parser.add_argument("--api-url", help="自定义行情地址(Binance风格klines)，默认进入实时模式")
    parser.add_argument("--symbol",
                        help="标的代码或中文名(黄金/原油/美元指数等)，缺省用数据源默认标的")
    parser.add_argument("--interval", default="1m",
                        help="K线周期，如 1m/5m/15m/30m/60m/1h/1d/1w/1M（1M=月线）")
    parser.add_argument("--limit", type=int,
                        help="取最近多少根K线，0=不限；省略时普通模式120、历史区间查询不限")
    parser.add_argument("--start", help="历史查询起始日期 YYYY-MM-DD")
    parser.add_argument("--end", help="历史查询结束日期 YYYY-MM-DD")
    parser.add_argument("--refresh", type=float, default=5.0,
                        help="实时刷新间隔秒数（仅 --live 生效）")
    parser.add_argument("--timeout", type=float, default=10.0,
                        help="单次请求超时秒数")
    parser.add_argument("--chart", default="candle", choices=["candle", "line"],
                        help="绘图方式：candle=K线蜡烛图(默认)，line=折线")
    parser.add_argument("--macd", default="12,26,9", metavar="FAST,SLOW,SIGNAL",
                        help="MACD 快线/慢线/信号线周期，默认 12,26,9")
    parser.add_argument("--rsi", default="14", metavar="N", help="RSI 周期，默认 14")
    parser.add_argument("--kdj", default="9,3,3", metavar="N,K,D",
                        help="KDJ 参数(RSV周期,K平滑,D平滑)，默认 9,3,3")
    parser.add_argument("--csv", metavar="FILE",
                        help="把当前K线导出为CSV(实时模式每帧覆盖写入)")
    return parser


def api_loader(args, limit, start, end, suffix):
    """自定义 REST 地址的数据读取器，返回 (标题, points)。"""

    def load():
        points = load_api_points(args.api_url, args.symbol, args.interval,
                                 limit, args.timeout, start, end)
        return "%s - %s%s" % (args.symbol or "BTCUSDT", args.interval, suffix), points

    return load


def source_loader(args, limit, start, end, suffix):
    """内置数据源的读取器，返回 (标题, points)。"""

    def load():
        name, code, points = resolve_points(args.source, args.symbol, args.interval,
                                            limit, args.timeout, start, end)
        interval = normalize_interval(args.interval)
        print("source=%s symbol=%s interval=%s bars=%d  %s"
              % (name, code, interval, len(points[0]), span_text(points)))
        return "%s [%s] %s%s" % (code, name, interval, suffix), points

    return load


def main(argv=None):
    parser = build_parser()
    argv = sys.argv[1:] if argv is None else argv
    if not argv:                       # 不带任何参数时只打印用法
        parser.print_help()
        return 0
    args = parser.parse_args(argv)

    # ---- 参数校验：在取数前给出明确提示，避免静默降级或反复刷错误 ----
    if args.live and args.history:
        parser.error("--live 与 --history 语义冲突（--live 持续刷新，--history 只查一次）")
    if args.api_url and args.source != "auto":
        parser.error("--api-url 与 --source 不能同时使用")
    if args.limit is not None and args.limit < 0:
        parser.error("--limit 不能为负数（0 表示不限）")
    if args.refresh <= 0:
        parser.error("--refresh 必须大于 0")
    if args.timeout <= 0:
        parser.error("--timeout 必须大于 0")
    if not args.api_url:
        names = [args.source] if args.source != "auto" else candidate_sources(args.symbol)
        wanted = normalize_interval(args.interval)
        if not any(wanted in SUPPORTED_INTERVALS.get(name, []) for name in names):
            parser.error("周期 %s 不被 %s 支持；%s 可用周期: %s"
                         % (wanted, "/".join(names), names[0],
                            "/".join(SUPPORTED_INTERVALS.get(names[0], []))))

    try:
        params = {"macd": parse_int_list(args.macd, 3, "--macd"),
                  "rsi": parse_int_list(args.rsi, 1, "--rsi"),
                  "kdj": parse_int_list(args.kdj, 3, "--kdj")}
    except ValueError as error:
        parser.error(str(error))
    if params["macd"][0] >= params["macd"][1]:
        parser.error("--macd 快线周期(%d)必须小于慢线周期(%d)"
                     % (params["macd"][0], params["macd"][1]))

    setup_font()
    setup_style()

    try:
        start = parse_date_arg(args.start)
        end = parse_date_arg(args.end)
    except ValueError as error:
        parser.error(str(error))
    if start and end and start > end:
        parser.error("--start(%s) 不能晚于 --end(%s)" % (start, end))

    historical = bool(start or end)
    # 普通模式默认120根；历史区间查询默认不限条数，否则只会拿到区间末尾一小段
    limit = args.limit if args.limit is not None else (0 if historical else 120)
    render = (partial(plot_candles, bars=None, params=params) if args.chart == "candle"
              else plot_points)
    suffix = range_suffix(start, end)
    # 联网条件：自定义地址 / --live / --history / 显式数据源 / 指定了标的
    # （--source auto --symbol 原油 这类写法显然是想查行情，不该退回本地文件）
    online = (bool(args.api_url) or args.live or args.history
              or args.source != "auto" or bool(args.symbol))
    # 持续刷新只由 --live 触发；--api-url 保持旧的实时语义，但指定历史区间时改为只查一次
    stream = args.live or (bool(args.api_url) and not historical)

    if not online:
        try:
            points = shape_points(load_file_points(args.file), start, end, limit)
        except (OSError, ValueError) as error:
            print("查询失败: %s" % error, file=sys.stderr)
            return 1
        render(points, "%s%s" % (args.file, suffix))
        print("file=%s bars=%d  %s" % (args.file, len(points[0]), span_text(points)))
        set_window_title(plt.gcf(), args.file)
        if args.csv:
            write_csv(points, args.csv)
        plt.show()
        return 0

    loader = (api_loader(args, limit, start, end, suffix) if args.api_url
              else source_loader(args, limit, start, end, suffix))
    if stream:
        stream_api(loader, args.refresh, args.chart, args.csv, params)
        return 0

    try:
        title, points = loader()
    except (OSError, ValueError, json.JSONDecodeError, KeyError) as error:
        print("查询失败: %s" % error, file=sys.stderr)
        return 1
    render(points, title)
    print("bars=%d  %s" % (len(points[0]), span_text(points)))
    set_window_title(plt.gcf(), title)
    if args.csv:
        write_csv(points, args.csv)
    plt.show()
    return 0


if __name__ == "__main__":
    sys.exit(main())
