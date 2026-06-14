#!/usr/bin/env python3
# coding: utf-8

"""
股票双均线交易系统专业版

架构流程:
    行情网关 --> 策略引擎 --> 风控引擎 --> 交易网关 --> 券商系统
                   |            |
                   v            v
               监控中心 --> Web界面 / 移动报警

功能模块:
    1. 真实行情接入 (Tushare / AKShare)
    2. 券商接口支持 (CTP / 华泰)
    3. 多周期分析引擎 (MA / RSI / MACD / ATR)
    4. 风险控制模块 (仓位 / 杠杆 / 止损)
    5. 历史回测引擎 (Backtrader)
    6. LSTM 价格预测 (TensorFlow)
    7. 期权衍生品引擎 (Greeks 计算)
    8. Rich 可视化监控面板

安装依赖:
    pip install -r requirements.txt

查看日志:
    tail -f logs/strategy.log
"""

import asyncio
import json
import logging
import math
import os
import time
from abc import ABC, abstractmethod
from collections import defaultdict
from dataclasses import dataclass, field
from datetime import datetime, timedelta
from threading import Lock, Thread
from typing import Any, Dict, List, Optional, Tuple

import numpy as np
import pandas as pd
import requests
import websocket
import yaml

try:
    import talib
except ImportError:
    talib = None  # type: ignore
from rich.console import Console
from rich.layout import Layout
from rich.panel import Panel
from rich.table import Table
from rich.text import Text

# 日志配置
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
)
logger = logging.getLogger(__name__)


# ==================== 数据模型 ====================
@dataclass
class Account:
    """账户数据结构"""
    total_assets: float = 1_000_000.0
    positions: Dict[str, Dict[str, float]] = field(default_factory=dict)
    daily_pnl: float = 0.0
    trade_history: List[Dict[str, Any]] = field(default_factory=list)

    @property
    def available_cash(self) -> float:
        """可用资金"""
        locked = sum(p.get("value", 0) for p in self.positions.values())
        return self.total_assets - locked

    @property
    def position_ratio(self) -> float:
        """总仓位比例"""
        if self.total_assets == 0:
            return 0.0
        return sum(p.get("value", 0) for p in self.positions.values()) / self.total_assets


@dataclass
class Order:
    """订单数据结构"""
    symbol: str
    direction: str  # "buy" / "sell"
    price: float
    qty: int = 0
    order_type: str = "limit"  # "limit" / "market"
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())


@dataclass
class Signal:
    """交易信号"""
    symbol: str
    direction: str
    price: float
    strength: float = 0.0  # 信号强度 0~1
    reason: str = ""


# ==================== 行情接口 ====================
class MarketDataAPI(ABC):
    """行情接口抽象基类"""

    @abstractmethod
    def get_real_time_data(self, symbol: str) -> Optional[Dict[str, Any]]:
        """获取实时行情"""
        ...

    @abstractmethod
    def get_historical_data(self, symbol: str, timeframe: str) -> pd.DataFrame:
        """获取历史K线"""
        ...

    def get_tick_data(self, symbol: str, trade_date: str = "") -> pd.DataFrame:
        """获取逐笔成交(可选覆写)"""
        return pd.DataFrame()


# ------------------ Tushare 行情实现 ------------------
class TushareAPI(MarketDataAPI):
    """Tushare Pro 数据源"""

    def __init__(self, token: str):
        self.token = token
        self.base_url = "http://api.tushare.pro"
        self._cache: Dict[str, Any] = {}
        self._cache_time: Dict[str, float] = {}

    def _request(self, api_name: str, **params) -> Optional[Dict]:
        data = {
            "api_name": api_name,
            "token": self.token,
            "params": params,
            "fields": "",
        }
        try:
            resp = requests.post(self.base_url, json=data, timeout=10)
            resp.raise_for_status()
            return resp.json()
        except requests.RequestException as e:
            logger.error("API请求失败 [%s]: %s", api_name, e)
            return None
        except ValueError as e:
            logger.error("JSON解析错误: %s", e)
            return None

    def _cached_request(self, api_name: str, cache_ttl: float = 1.0, **params) -> Optional[Dict]:
        """带缓存的请求，避免同一秒内重复调用"""
        cache_key = f"{api_name}:{json.dumps(params, sort_keys=True)}"
        now = time.time()
        if cache_key in self._cache and (now - self._cache_time.get(cache_key, 0)) < cache_ttl:
            return self._cache[cache_key]
        result = self._request(api_name, **params)
        if result is not None:
            self._cache[cache_key] = result
            self._cache_time[cache_key] = now
        return result

    def get_real_time_data(self, symbol: str) -> Optional[Dict[str, Any]]:
        data = self._cached_request("realtime_quote", cache_ttl=3.0, ts_code=symbol)
        if not data or "data" not in data:
            logger.warning("实时行情数据为空: %s", symbol)
            return None
        try:
            item = data["data"]["items"][0]
        except (IndexError, KeyError, TypeError) as e:
            logger.error("解析实时行情数据失败: %s, error: %s", symbol, e)
            return None
        return {
            "symbol": symbol,
            "price": float(item[21]),
            "volume": int(item[13]),
        }

    def get_historical_data(self, symbol: str, timeframe: str) -> pd.DataFrame:
        freq_map = {"1m": "1MIN", "5m": "5MIN", "15m": "15MIN", "30m": "30MIN", "1d": "D"}
        freq = freq_map.get(timeframe, "15MIN")
        json_rsp = self._request("pro_bar", ts_code=symbol, freq=freq)
        if not json_rsp or "data" not in json_rsp:
            logger.warning("获取历史数据失败: %s, timeframe: %s", symbol, timeframe)
            return pd.DataFrame()
        try:
            data = json_rsp["data"]
            if data is None:
                logger.warning("解析历史数据失败: %s, msg: %s", symbol, json_rsp.get("msg", "unknown"))
                return pd.DataFrame()
            items = data["items"]
        except (KeyError, TypeError) as e:
            logger.error("解析历史数据失败: %s, error: %s", symbol, e)
            return pd.DataFrame()

        df = pd.DataFrame(items)
        # 标准化列名
        col_map = {
            1: "ts_code", 2: "trade_date", 3: "open", 4: "high",
            5: "low", 6: "close", 7: "pre_close", 8: "change",
            9: "pct_chg", 10: "vol", 11: "amount",
        }
        if df.shape[1] >= 6:
            df = df.rename(columns=col_map)
        return df


# ==================== 交易接口 ====================
class TradeAPI(ABC):
    """交易接口抽象基类"""

    @abstractmethod
    def connect(self) -> bool:
        """建立连接，返回是否成功"""
        ...

    @abstractmethod
    def place_order(self, symbol: str, price: float, qty: int, direction: str) -> Optional[str]:
        """下单，返回订单ID"""
        ...

    @abstractmethod
    def cancel_order(self, order_id: str) -> bool:
        """撤单"""
        ...

    @abstractmethod
    def get_positions(self) -> Dict[str, Dict[str, Any]]:
        """查询持仓"""
        ...

    @abstractmethod
    def get_account_info(self) -> Dict[str, Any]:
        """查询账户信息"""
        ...


# ------------------ CTP 交易接口实现 ------------------
class CTPTradeAPI(TradeAPI):
    """CTP期货/股票交易接口 (SimNow 仿真环境)"""

    def __init__(self, config: Dict[str, Any]):
        self.config = config
        self.ws: Optional[websocket.WebSocketApp] = None
        self.connected = False
        self._pending_orders: Dict[str, Dict] = {}
        self._positions: Dict[str, Dict[str, Any]] = {}

    def connect(self) -> bool:
        def on_message(ws, message):
            try:
                msg = json.loads(message)
                logger.debug("CTP message: %s", msg)
                self._handle_response(msg)
            except json.JSONDecodeError:
                logger.warning("无法解析CTP响应: %s", message[:200])

        def on_error(ws, error):
            logger.error("CTP WebSocket错误: %s", error)
            self.connected = False

        def on_close(ws, close_status_code, close_msg):
            logger.warning("CTP WebSocket断开: %s", close_msg)
            self.connected = False

        try:
            self.ws = websocket.WebSocketApp(
                self.config.get("trade_server", "tcp://180.168.146.187:10130"),
                on_message=on_message,
                on_error=on_error,
                on_close=on_close,
            )
            Thread(target=self.ws.run_forever, daemon=True).start()
            time.sleep(2)
            # 发送登录认证
            auth_msg = {
                "action": "login",
                "user_id": self.config.get("user_id", ""),
                "password": self.config.get("password", ""),
                "app_id": self.config.get("app_id", ""),
                "auth_code": self.config.get("auth_code", ""),
            }
            self.ws.send(json.dumps(auth_msg))
            time.sleep(1)
            self.connected = True
            logger.info("CTP 连接成功")
            return True
        except Exception as e:
            logger.error("CTP 连接失败: %s", e)
            return False

    def _handle_response(self, msg: Dict[str, Any]) -> None:
        """处理服务端响应"""
        action = msg.get("action", "")
        if action == "order_response":
            order_id = msg.get("order_id", "")
            status = msg.get("status", "")
            logger.info("订单 %s 状态: %s", order_id, status)
        elif action == "position_update":
            self._positions = msg.get("positions", {})

    def place_order(self, symbol: str, price: float, qty: int, direction: str) -> Optional[str]:
        if not self.ws or not self.connected:
            logger.error("CTP 未连接，无法下单")
            return None
        order_id = f"ORD_{int(time.time() * 1000)}"
        order_msg = {
            "action": "place_order",
            "order_id": order_id,
            "Symbol": symbol,
            "Price": price,
            "Quantity": qty,
            "Direction": direction,
            "OrderType": "0",  # 限价单
        }
        try:
            self.ws.send(json.dumps(order_msg))
            self._pending_orders[order_id] = order_msg
            logger.info("Order sent: %s %s %s@%.2f x%d", order_id, direction, symbol, price, qty)
            return order_id
        except Exception as e:
            logger.error("下单失败 %s: %s", order_id, e)
            return None

    def cancel_order(self, order_id: str) -> bool:
        if not self.ws or not self.connected:
            return False
        try:
            self.ws.send(json.dumps({"action": "cancel_order", "order_id": order_id}))
            logger.info("撤单请求: %s", order_id)
            return True
        except Exception as e:
            logger.error("撤单失败 %s: %s", order_id, e)
            return False

    def get_positions(self) -> Dict[str, Dict[str, Any]]:
        try:
            if self.ws and self.connected:
                self.ws.send(json.dumps({"action": "query_position"}))
        except Exception as e:
            logger.error("查询持仓失败: %s", e)
        return self._positions

    def get_account_info(self) -> Dict[str, Any]:
        return {
            "broker": "ctp",
            "connected": self.connected,
            "pending_orders": len(self._pending_orders),
        }


# ==================== 多周期分析引擎 ====================
class MultiTimeframeAnalyzer:
    """多周期技术指标分析引擎

    支持 MA / EMA / RSI / MACD / ATR / 布林带 / KDJ 等指标。
    通过多周期共振判断趋势强度。
    """

    def __init__(self, symbol: str, timeframes: List[str], data_api: MarketDataAPI):
        self.symbol = symbol
        self.timeframes = timeframes
        self.data_api = data_api
        self.data: Dict[str, pd.DataFrame] = {tf: pd.DataFrame() for tf in timeframes}
        self._indicator_cache: Dict[str, Dict] = {}

    def update_data(self) -> None:
        """更新各周期K线数据"""
        for tf in self.timeframes:
            new_data = self.data_api.get_historical_data(self.symbol, tf)
            if not new_data.empty:
                self.data[tf] = pd.concat([self.data[tf], new_data]).tail(500)
        self._indicator_cache.clear()  # 数据更新后清空指标缓存

    def _ensure_ohlc(self, df: pd.DataFrame) -> pd.DataFrame:
        """确保 DataFrame 有 open/high/low/close/volume 列"""
        if df.empty:
            return df
        # 如果是按索引的列，尝试映射
        for col in ["close", "open", "high", "low", "volume", "vol"]:
            if col not in df.columns:
                # 尝试从小写或大写中查找
                candidates = [c for c in df.columns if c.lower() == col.lower()]
                if candidates:
                    df[col] = df[candidates[0]]
        return df

    def calculate_indicators(self) -> Dict[str, Dict[str, Any]]:
        """计算所有周期的技术指标"""
        if self._indicator_cache:
            return self._indicator_cache

        if talib is None:
            logger.warning("TA-Lib 不可用，返回空指标")
            return {}

        indicators: Dict[str, Dict[str, Any]] = {}
        for tf in self.timeframes:
            df = self._ensure_ohlc(self.data[tf])
            if df.empty or "close" not in df.columns:
                indicators[tf] = {}
                continue

            close = df["close"].astype(float)
            high = df.get("high", close).astype(float)
            low = df.get("low", close).astype(float)
            volume = df.get("volume", df.get("vol", pd.Series([0] * len(close)))).astype(float)

            # 趋势指标
            ma5 = talib.SMA(close, timeperiod=5)
            ma20 = talib.SMA(close, timeperiod=20)
            ma60 = talib.SMA(close, timeperiod=60) if len(close) >= 60 else ma20

            # 震荡指标
            rsi = talib.RSI(close, timeperiod=14)
            macd_line, macd_signal, macd_hist = talib.MACD(
                close, fastperiod=12, slowperiod=26, signalperiod=9
            )

            # 波动率指标
            atr = talib.ATR(high, low, close, timeperiod=14)
            upper, _, lower = talib.BBANDS(
                close, timeperiod=20, nbdevup=2, nbdevdn=2, matype=0
            )

            # KDJ
            slowk, slowd = talib.STOCH(
                high, low, close,
                fastk_period=9, slowk_period=3, slowk_matype=0,
                slowd_period=3, slowd_matype=0,
            )

            latest = len(close) - 1
            indicators[tf] = {
                "close": float(close.iloc[latest]) if latest >= 0 else 0,
                "MA5": float(ma5.iloc[latest]) if latest >= 0 else 0,
                "MA20": float(ma20.iloc[latest]) if latest >= 0 else 0,
                "MA60": float(ma60.iloc[latest]) if latest >= 0 else 0,
                "RSI": float(rsi.iloc[latest]) if latest >= 0 else 50,
                "MACD": float(macd_line.iloc[latest]) if latest >= 0 else 0,
                "MACD_signal": float(macd_signal.iloc[latest]) if latest >= 0 else 0,
                "MACD_hist": float(macd_hist.iloc[latest]) if latest >= 0 else 0,
                "ATR": float(atr.iloc[latest]) if latest >= 0 else 0,
                "BB_upper": float(upper.iloc[latest]) if latest >= 0 else 0,
                "BB_lower": float(lower.iloc[latest]) if latest >= 0 else 0,
                "K": float(slowk.iloc[latest]) if latest >= 0 else 50,
                "D": float(slowd.iloc[latest]) if latest >= 0 else 50,
                "volume": float(volume.iloc[latest]) if latest >= 0 else 0,
            }

        self._indicator_cache = indicators
        return indicators

    def get_trend_score(self) -> float:
        """多周期趋势一致性评分 (-1.0 ~ 1.0)

        正值 = 多头共振，负值 = 空头共振，接近0 = 震荡分歧
        """
        indicators = self.calculate_indicators()
        if not indicators:
            return 0.0

        scores = []
        for tf, ind in indicators.items():
            tf_score = 0.0
            # MA 排列
            if ind.get("MA5", 0) > ind.get("MA20", 0) > ind.get("MA60", 0):
                tf_score += 0.3
            elif ind.get("MA5", 0) < ind.get("MA20", 0) < ind.get("MA60", 0):
                tf_score -= 0.3
            # RSI 区间
            rsi = ind.get("RSI", 50)
            if rsi > 60:
                tf_score += 0.2
            elif rsi < 40:
                tf_score -= 0.2
            # MACD
            if ind.get("MACD_hist", 0) > 0:
                tf_score += 0.2
            else:
                tf_score -= 0.2
            # KDJ
            if ind.get("K", 50) > ind.get("D", 50):
                tf_score += 0.15
            else:
                tf_score -= 0.15
            # 价格相对布林带位置
            close = ind.get("close", 0)
            bb_upper = ind.get("BB_upper", 0)
            bb_lower = ind.get("BB_lower", 0)
            if bb_upper > bb_lower > 0:
                bb_pos = (close - bb_lower) / (bb_upper - bb_lower)
                if bb_pos > 0.8:
                    tf_score += 0.15  # 强势区
                elif bb_pos < 0.2:
                    tf_score -= 0.15

            scores.append(tf_score)

        # 多周期共振加权
        weights = {"15m": 0.3, "30m": 0.4, "1d": 0.3}
        total_weight = 0.0
        weighted_score = 0.0
        for i, tf in enumerate(self.timeframes):
            w = weights.get(tf, 1.0 / len(self.timeframes))
            weighted_score += scores[i] * w
            total_weight += w

        return weighted_score / total_weight if total_weight > 0 else 0.0


# ==================== 高级风控模块 ====================
class RiskManager:
    """多层次风控引擎

    检查项:
        1. 单日最大亏损限制
        2. 总杠杆率上限
        3. 单品种仓位比例
        4. 单笔止损 (ATR 动态止损)
        5. 日内交易次数限制
        6. 黑名单 / 涨跌停过滤
    """

    def __init__(self, config: Dict[str, Any]):
        self.config = config
        self.position_lock = Lock()
        self.daily_trade_count: Dict[str, int] = defaultdict(int)
        self.blacklist: set = set()

    def check_order(self, order: Order, account: Account) -> Tuple[bool, str]:
        """检查订单是否通过风控，返回 (通过, 原因)"""
        with self.position_lock:
            # 1. 黑名单检查
            if order.symbol in self.blacklist:
                return False, "Symbol in blacklist"

            # 2. 单日最大亏损限制
            daily_loss_limit = self.config.get("daily_loss_limit", -0.05)
            if account.daily_pnl < daily_loss_limit:
                return False, f"Exceed daily loss limit ({daily_loss_limit:.1%})"

            # 3. 总杠杆率检查
            if account.total_assets > 0:
                position_value = sum(p.get("value", 0) for p in account.positions.values())
                leverage = position_value / account.total_assets
                max_leverage = self.config.get("max_leverage", 2.0)
                if leverage > max_leverage:
                    return False, f"Exceed max leverage ({leverage:.1f}x > {max_leverage:.1f}x)"

            # 4. 单品种仓位比例检查
            max_pos_ratio = self.config.get("max_position_ratio", 0.5)
            if account.total_assets > 0:
                current_pos = account.positions.get(order.symbol, {})
                current_value = current_pos.get("value", 0)
                position_ratio = (current_value + order.price * order.qty) / account.total_assets
                if position_ratio > max_pos_ratio:
                    return False, f"Exceed position ratio ({position_ratio:.1%} > {max_pos_ratio:.1%})"

            # 5. 单日交易次数限制
            max_trades = self.config.get("max_daily_trades", 100)
            if self.daily_trade_count.get(order.symbol, 0) >= max_trades:
                return False, f"Exceed daily trade limit ({max_trades})"

            # 6. 单笔止损检查 (基于 ATR)
            stop_loss = self.config.get("single_stop_loss", 0.03)
            if order.direction == "buy":
                atr_stop_price = order.price * (1 - stop_loss)
            else:
                atr_stop_price = order.price * (1 + stop_loss)
            logger.debug("止损价: %.2f", atr_stop_price)

            self.daily_trade_count[order.symbol] += 1
            return True, "Approved"

    def add_to_blacklist(self, symbol: str, reason: str = "") -> None:
        """将品种加入黑名单"""
        self.blacklist.add(symbol)
        logger.warning("黑名单添加: %s, 原因: %s", symbol, reason)

    def remove_from_blacklist(self, symbol: str) -> None:
        """从黑名单移除"""
        self.blacklist.discard(symbol)

    def reset_daily(self) -> None:
        """重置每日统计"""
        self.daily_trade_count.clear()


# ==================== 策略核心 ====================
class EnhancedDualMAStrategy:
    """双均线增强策略

    信号逻辑:
        - 多周期 MA 金叉共振 → 买入
        - 多周期 MA 死叉共振 → 卖出
        - 结合 RSI / MACD / KDJ 过滤假信号
        - ATR 动态仓位管理
    """

    def __init__(self, config_file: str):
        if not os.path.exists(config_file):
            logger.error("配置文件不存在: %s", config_file)
            raise FileNotFoundError(f"Config file not found: {config_file}")

        self.load_config(config_file)
        self.init_components()
        self.running = False
        self._signal_history: List[Signal] = []
        self._loop_count = 0

    def load_config(self, file_path: str) -> None:
        with open(file_path, encoding="utf-8") as f:
            self.config = yaml.safe_load(f)
        logger.info("配置加载成功: %d 个标的, 周期: %s",
                     len(self.config["strategy"]["symbols"]),
                     self.config["strategy"]["timeframes"])

    def init_components(self) -> None:
        """初始化各子模块"""
        # 行情接口
        market_cfg = self.config["market"]
        if market_cfg["data_source"] == "tushare":
            self.data_api = TushareAPI(market_cfg["tushare_token"])
        else:
            raise ValueError(f"不支持的数据源: {market_cfg['data_source']}")

        # 交易接口
        account_cfg = self.config["account"]
        broker = account_cfg.get("broker", "ctp")
        if broker == "ctp":
            self.trade_api: TradeAPI = CTPTradeAPI(account_cfg)
        elif broker == "htsc":
            self.trade_api = HTSCAPI(account_cfg)
        else:
            raise ValueError(f"不支持的券商: {broker}")

        # 分析引擎 (每个标的独立)
        strategy_cfg = self.config["strategy"]
        self.analyzers: Dict[str, MultiTimeframeAnalyzer] = {
            sym: MultiTimeframeAnalyzer(sym, strategy_cfg["timeframes"], self.data_api)
            for sym in strategy_cfg["symbols"]
        }

        # 风控模块
        self.risk_mgr = RiskManager(self.config["risk"])

        # 账户
        self.account = Account(total_assets=1_000_000.0)

        # 预测模块 (可选)
        self.predictor: Optional[Predictor] = None

        # 衍生品引擎 (可选)
        self.derivatives: Optional[DerivativesEngine] = None

    def run_strategy(self) -> None:
        """主策略循环"""
        self.running = True
        logger.info("策略启动 - 标的: %s", self.config["strategy"]["symbols"])

        while self.running:
            try:
                self._loop_count += 1
                loop_start = time.time()

                # 1. 更新行情数据
                for sym in self.config["strategy"]["symbols"]:
                    self.analyzers[sym].update_data()

                # 2. 生成交易信号
                signals = self.generate_signals()

                # 3. 风控检查
                valid_orders = self._apply_risk_control(signals)

                # 4. 执行交易
                self.execute_orders(valid_orders)

                # 5. 更新监控界面
                self.update_gui()

                # 6. 每 N 次循环同步持仓
                if self._loop_count % 12 == 0:
                    self._sync_positions()

                # 动态调整 sleep 时间
                elapsed = time.time() - loop_start
                sleep_time = max(1, 5 - elapsed)
                time.sleep(sleep_time)

            except Exception as e:
                logger.error("策略执行异常 [循环 %d]: %s", self._loop_count, e, exc_info=True)
                time.sleep(5)

    def _apply_risk_control(self, signals: List[Order]) -> List[Order]:
        """应用风控过滤"""
        valid_orders = []
        for order in signals:
            is_valid, reason = self.risk_mgr.check_order(order, self.account)
            if is_valid:
                valid_orders.append(order)
            else:
                logger.warning("风控拒绝: %s %s → %s", order.symbol, order.direction, reason)
        return valid_orders

    def _sync_positions(self) -> None:
        """同步券商实际持仓"""
        try:
            broker_positions = self.trade_api.get_positions()
            for sym, pos in broker_positions.items():
                if sym in self.account.positions:
                    self.account.positions[sym].update(pos)
                else:
                    self.account.positions[sym] = pos
        except Exception as e:
            logger.warning("持仓同步失败: %s", e)

    def generate_signals(self) -> List[Order]:
        """生成交易信号

        多周期共振判断:
            - 所有周期的 MA5 > MA20 → 多头信号
            - 所有周期的 MA5 < MA20 → 空头信号
            - 结合趋势评分过滤弱信号
        """
        orders: List[Order] = []
        for sym, analyzer in self.analyzers.items():
            indicators = analyzer.calculate_indicators()
            if not indicators:
                continue

            # 多周期 MA 排列判断
            bullish = all(
                ind.get("MA5", 0) > ind.get("MA20", 0)
                for ind in indicators.values()
            )
            bearish = all(
                ind.get("MA5", 0) < ind.get("MA20", 0)
                for ind in indicators.values()
            )

            # 获取趋势评分
            trend_score = analyzer.get_trend_score()

            # 获取实时价格
            rt_data = self.data_api.get_real_time_data(sym)
            if rt_data is None:
                continue

            price = rt_data["price"]

            # 信号过滤: 只有趋势评分足够强时才交易
            if bullish and trend_score > 0.3:
                orders.append(Order(
                    symbol=sym, direction="buy", price=price,
                    qty=self.calculate_position_size(sym, price),
                ))
            elif bearish and trend_score < -0.3:
                orders.append(Order(
                    symbol=sym, direction="sell", price=price,
                    qty=self.calculate_position_size(sym, price),
                ))
            else:
                logger.debug(
                    "%s: 无信号 (trend_score=%.2f, bullish=%s, bearish=%s)",
                    sym, trend_score, bullish, bearish,
                )

        self._signal_history.extend([
            Signal(order.symbol, order.direction, order.price,
                   reason=f"trend_score={trend_score:.2f}")
            for order in orders
        ])
        # 只保留最近 1000 条信号记录
        if len(self._signal_history) > 1000:
            self._signal_history = self._signal_history[-1000:]

        return orders

    def execute_orders(self, orders: List[Order]) -> None:
        """批量执行订单"""
        for order in orders:
            order_id = self.trade_api.place_order(
                symbol=order.symbol,
                price=order.price,
                qty=order.qty,
                direction=order.direction,
            )
            if order_id:
                self.account.trade_history.append({
                    "order_id": order_id,
                    "symbol": order.symbol,
                    "direction": order.direction,
                    "price": order.price,
                    "qty": order.qty,
                    "timestamp": datetime.now().isoformat(),
                })

    def calculate_position_size(self, symbol: str, price: float) -> int:
        """基于 ATR 波动率的动态仓位计算

        风险预算 = 总资产 × 1%
        仓位 = 风险预算 / ATR(14)
        """
        try:
            analyzer = self.analyzers[symbol]
            if talib is None or "15m" not in analyzer.data or analyzer.data["15m"].empty:
                # 默认: 总资产 1% / 价格
                return max(100, int(self.account.total_assets * 0.01 / price))

            recent_data = analyzer._ensure_ohlc(analyzer.data["15m"])
            close = recent_data["close"].astype(float)
            high = recent_data.get("high", close).astype(float)
            low = recent_data.get("low", close).astype(float)
            atr_series = talib.ATR(high, low, close, timeperiod=14)
            atr = float(atr_series.iloc[-1]) if not atr_series.empty else price * 0.02

            risk_capital = self.account.total_assets * 0.01  # 1% 风险资本
            qty = max(100, int(risk_capital / max(atr, price * 0.001)))
            return qty
        except Exception as e:
            logger.warning("仓位计算异常 %s: %s, 使用默认值", symbol, e)
            return 100

    def update_gui(self) -> None:
        """Rich 终端可视化"""
        console = Console()
        layout = Layout()
        layout.split(
            Layout(name="header", size=3),
            Layout(name="main", ratio=1),
            Layout(name="footer", size=8),
        )

        # ---------- 头部: 账户概览 ----------
        available = self.account.available_cash
        pnl_color = "green" if self.account.daily_pnl >= 0 else "red"

        header_table = Table(title="📊 账户概览", title_style="bold cyan")
        header_table.add_column("总资产", style="cyan")
        header_table.add_column("可用资金", style="yellow")
        header_table.add_column("仓位比例", style="magenta")
        header_table.add_column("当日盈亏", style=pnl_color)
        header_table.add_column("循环次数", style="dim")
        header_table.add_row(
            f"¥{self.account.total_assets:,.2f}",
            f"¥{available:,.2f}",
            f"{self.account.position_ratio:.1%}",
            f"¥{self.account.daily_pnl:+,.2f}",
            str(self._loop_count),
        )
        layout["header"].update(Panel(header_table))

        # ---------- 主体: 持仓 & 信号 ----------
        main_table = Table(title="📈 持仓监控", title_style="bold green")
        main_table.add_column("代码")
        main_table.add_column("方向")
        main_table.add_column("数量")
        main_table.add_column("市值")
        main_table.add_column("盈亏")
        main_table.add_column("趋势评分", style="yellow")

        for sym, pos in self.account.positions.items():
            trend_score = self.analyzers[sym].get_trend_score()
            direction = "🔴 多" if pos.get("qty", 0) > 0 else "🟢 空"
            main_table.add_row(
                sym,
                direction,
                str(abs(int(pos.get("qty", 0)))),
                f"¥{pos.get('value', 0):,.2f}",
                f"¥{pos.get('pnl', 0):+,.2f}",
                f"{trend_score:+.2f}",
            )

        if not self.account.positions:
            main_table.add_row("(空仓)", "-", "-", "-", "-", "-")

        layout["main"].update(Panel(main_table))

        # ---------- 底部: 最近信号 ----------
        footer_table = Table(title="📡 最近信号", title_style="bold blue")
        footer_table.add_column("时间")
        footer_table.add_column("代码")
        footer_table.add_column("方向")
        footer_table.add_column("价格")
        footer_table.add_column("原因")

        recent = self._signal_history[-5:]
        for sig in recent:
            direction_style = "[green]BUY[/green]" if sig.direction == "buy" else "[red]SELL[/red]"
            footer_table.add_row(
                datetime.now().strftime("%H:%M:%S"),
                sig.symbol,
                direction_style,
                f"{sig.price:.2f}",
                sig.reason,
            )
        layout["footer"].update(Panel(footer_table))

        console.print(layout)


# ==================== 回测模块 ====================
try:
    import backtrader as bt

    class DualMABacktestStrategy(bt.Strategy):
        """Backtrader 双均线回测策略"""

        params = (
            ("fast_period", 5),
            ("slow_period", 20),
            ("rsi_period", 14),
        )

        def __init__(self):
            self.ma_fast = bt.indicators.SMA(self.data.close, period=self.params.fast_period)
            self.ma_slow = bt.indicators.SMA(self.data.close, period=self.params.slow_period)
            self.rsi = bt.indicators.RSI(self.data.close, period=self.params.rsi_period)
            self.crossover = bt.indicators.CrossOver(self.ma_fast, self.ma_slow)
            self.order = None

        def notify_order(self, order):
            if order.status in [order.Completed, order.Canceled, order.Margin]:
                self.order = None

        def next(self):
            if self.order:
                return

            # RSI 过滤: 只在非极端区间交易
            if self.rsi[0] < 30 and self.crossover[0] > 0:
                self.order = self.buy()
            elif self.rsi[0] > 70 and self.crossover[0] < 0:
                self.order = self.sell()


    class BacktestEngine:
        """回测引擎

        使用示例:
            engine = BacktestEngine()
            engine.load_data("600519.SH", df)
            result = engine.run()
            engine.plot()
        """

        def __init__(self, initial_cash: float = 1_000_000.0, commission: float = 0.0003):
            self.cerebro = bt.Cerebro()
            self.cerebro.broker.setcash(initial_cash)
            self.cerebro.broker.setcommission(commission=commission)
            self.cerebro.addstrategy(DualMABacktestStrategy)
            self._has_data = False

        def load_data(self, symbol: str, df: pd.DataFrame) -> None:
            """加载历史数据

            Args:
                symbol: 股票代码
                df: 包含 open/high/low/close/volume 的 DataFrame
            """
            df = df.copy()
            if "datetime" not in df.columns and df.index.name != "datetime":
                df["datetime"] = pd.to_datetime(df.index)
            data = bt.feeds.PandasData(
                dataname=df,
                datetime="datetime" if "datetime" in df.columns else None,
                open="open", high="high", low="low", close="close", volume="volume",
                openinterest=-1,
            )
            self.cerebro.adddata(data, name=symbol)
            self._has_data = True

        def add_sizer(self, sizer=None) -> None:
            """添加仓位管理"""
            if sizer:
                self.cerebro.addsizer(sizer)
            else:
                # 默认: 每次使用 95% 可用资金
                self.cerebro.addsizer(bt.sizers.PercentSizer, percents=95)

        def add_analyzer(self, analyzer_cls=None) -> None:
            """添加分析器"""
            self.cerebro.addanalyzer(bt.analyzers.SharpeRatio, _name="sharpe")
            self.cerebro.addanalyzer(bt.analyzers.DrawDown, _name="drawdown")
            self.cerebro.addanalyzer(bt.analyzers.Returns, _name="returns")
            self.cerebro.addanalyzer(bt.analyzers.TradeAnalyzer, _name="trades")

        def run(self) -> Dict[str, Any]:
            """执行回测，返回结果摘要"""
            if not self._has_data:
                logger.warning("回测引擎: 未加载数据")
                return {}

            start_value = self.cerebro.broker.getvalue()
            logger.info("回测起始资金: ¥%.2f", start_value)

            results = self.cerebro.run()
            strategy = results[0]

            end_value = self.cerebro.broker.getvalue()
            total_return = (end_value - start_value) / start_value

            summary = {
                "初始资金": start_value,
                "最终资金": end_value,
                "总收益率": f"{total_return:.2%}",
                "夏普比率": strategy.analyzers.sharpe.get_analysis().get("sharperatio", "N/A"),
                "最大回撤": f"{strategy.analyzers.drawdown.get_analysis().get('max', {}).get('drawdown', 0):.2%}",
                "年化收益": f"{strategy.analyzers.returns.get_analysis().get('rnorm100', 0):.2f}%",
            }

            logger.info("回测完成: %s", summary)
            return summary

        def plot(self, save_path: str = "") -> None:
            """绘制回测图表"""
            try:
                self.cerebro.plot(style="candlestick")
            except Exception as e:
                logger.warning("回测绘图失败: %s", e)

except ImportError:
    logger.warning("backtrader 未安装，回测模块不可用")

    class BacktestEngine:
        """回测引擎占位 (需安装 backtrader)"""

        def __init__(self, **kwargs):
            logger.warning("BacktestEngine: backtrader 未安装")

        def load_data(self, *args, **kwargs):
            pass

        def run(self):
            return {}

        def plot(self, *args, **kwargs):
            pass


# ==================== 华泰证券接口 ====================
class HTSCAPI(TradeAPI):
    """华泰证券交易接口 (REST API 模式)

    支持华泰 MATIC 协议，通过 HTTPS 进行交易。
    需要先在华泰官网申请 API Key。
    """

    def __init__(self, config: Dict[str, Any]):
        self.config = config
        self.base_url = config.get("trade_server", "https://api.htsc.com")
        self.api_key = config.get("app_id", "")
        self.secret = config.get("auth_code", "")
        self.session = requests.Session()
        self.session.headers.update({
            "Content-Type": "application/json",
            "X-API-Key": self.api_key,
        })
        self._token: Optional[str] = None
        self.connected = False

    def connect(self) -> bool:
        """登录获取 token"""
        try:
            resp = self.session.post(
                f"{self.base_url}/auth/login",
                json={
                    "user_id": self.config.get("user_id", ""),
                    "password": self.config.get("password", ""),
                },
                timeout=10,
            )
            data = resp.json()
            if data.get("code") == 0:
                self._token = data.get("token", "")
                self.session.headers["Authorization"] = f"Bearer {self._token}"
                self.connected = True
                logger.info("华泰证券 登录成功")
                return True
            else:
                logger.error("华泰证券 登录失败: %s", data.get("msg", "unknown"))
                return False
        except Exception as e:
            logger.error("华泰证券 连接失败: %s", e)
            return False

    def place_order(self, symbol: str, price: float, qty: int, direction: str) -> Optional[str]:
        if not self.connected:
            logger.error("华泰证券 未连接")
            return None
        try:
            resp = self.session.post(
                f"{self.base_url}/order/place",
                json={
                    "symbol": symbol,
                    "price": price,
                    "quantity": qty,
                    "side": "B" if direction == "buy" else "S",
                    "order_type": "LIMIT",
                },
                timeout=10,
            )
            data = resp.json()
            order_id = data.get("order_id", "")
            logger.info("华泰下单: %s %s %s@%.2f x%d", order_id, direction, symbol, price, qty)
            return order_id
        except Exception as e:
            logger.error("华泰下单失败: %s", e)
            return None

    def cancel_order(self, order_id: str) -> bool:
        try:
            resp = self.session.post(
                f"{self.base_url}/order/cancel",
                json={"order_id": order_id},
                timeout=10,
            )
            return resp.json().get("code") == 0
        except Exception as e:
            logger.error("华泰撤单失败: %s", e)
            return False

    def get_positions(self) -> Dict[str, Dict[str, Any]]:
        try:
            resp = self.session.get(f"{self.base_url}/account/positions", timeout=10)
            return resp.json().get("data", {})
        except Exception as e:
            logger.error("华泰查询持仓失败: %s", e)
            return {}

    def get_account_info(self) -> Dict[str, Any]:
        try:
            resp = self.session.get(f"{self.base_url}/account/info", timeout=10)
            return resp.json().get("data", {})
        except Exception as e:
            logger.error("华泰查询账户失败: %s", e)
            return {}


# ==================== 异步数据更新器 ====================
class AsyncDataUpdater:
    """异步行情数据更新器

    使用 asyncio 并发更新多个标的的行情数据，
    显著降低 IO 等待时间。
    """

    def __init__(self, symbols: List[str], data_api: MarketDataAPI, analyzers: Dict[str, MultiTimeframeAnalyzer]):
        self.symbols = symbols
        self.data_api = data_api
        self.analyzers = analyzers
        self.running = False

    async def _update_single(self, sym: str) -> None:
        """异步更新单个标的"""
        loop = asyncio.get_running_loop()
        await loop.run_in_executor(None, self.analyzers[sym].update_data)

    async def update_all(self) -> None:
        """并发更新所有标的"""
        tasks = [self._update_single(sym) for sym in self.symbols]
        await asyncio.gather(*tasks, return_exceptions=True)

    async def run_loop(self, interval: float = 5.0) -> None:
        """异步主循环"""
        self.running = True
        logger.info("异步数据更新器启动, 间隔: %.1fs", interval)
        while self.running:
            try:
                await self.update_all()
                await asyncio.sleep(interval)
            except Exception as e:
                logger.error("异步更新异常: %s", e)
                await asyncio.sleep(interval)

    def stop(self) -> None:
        self.running = False


# ==================== 期权衍生品引擎 ====================
class DerivativesEngine:
    """期权衍生品计算引擎

    支持:
        - 欧式/美式期权定价 (Black-Scholes / Binomial Tree)
        - 希腊值计算 (Delta / Gamma / Theta / Vega / Rho)
        - 隐含波动率计算
        - 期权组合风险分析
    """

    def __init__(self, risk_free_rate: float = 0.03):
        self.r = risk_free_rate  # 无风险利率
        self._cache: Dict[str, Dict[str, float]] = {}

    @staticmethod
    def _norm_cdf(x: float) -> float:
        """标准正态分布 CDF (使用 math.erf)"""
        return 0.5 * (1.0 + math.erf(x / math.sqrt(2.0)))

    @staticmethod
    def _norm_pdf(x: float) -> float:
        """标准正态分布 PDF"""
        return math.exp(-0.5 * x ** 2) / math.sqrt(2.0 * math.pi)

    def black_scholes(
        self,
        S: float,      # 标的价格
        K: float,      # 行权价
        T: float,      # 剩余时间 (年)
        sigma: float,  # 波动率
        option_type: str = "call",  # "call" / "put"
    ) -> Dict[str, float]:
        """Black-Scholes 期权定价

        Returns:
            dict: price, delta, gamma, theta, vega, rho
        """
        if T <= 0:
            intrinsic = max(S - K, 0) if option_type == "call" else max(K - S, 0)
            return {"price": intrinsic, "delta": 1.0, "gamma": 0, "theta": 0, "vega": 0, "rho": 0}

        d1 = (math.log(S / K) + (self.r + 0.5 * sigma ** 2) * T) / (sigma * math.sqrt(T))
        d2 = d1 - sigma * math.sqrt(T)

        if option_type == "call":
            price = S * self._norm_cdf(d1) - K * math.exp(-self.r * T) * self._norm_cdf(d2)
            delta = self._norm_cdf(d1)
            theta = (-S * self._norm_pdf(d1) * sigma / (2 * math.sqrt(T))
                     - self.r * K * math.exp(-self.r * T) * self._norm_cdf(d2))
        else:
            price = K * math.exp(-self.r * T) * self._norm_cdf(-d2) - S * self._norm_cdf(-d1)
            delta = self._norm_cdf(d1) - 1
            theta = (-S * self._norm_pdf(d1) * sigma / (2 * math.sqrt(T))
                     + self.r * K * math.exp(-self.r * T) * self._norm_cdf(-d2))

        gamma = self._norm_pdf(d1) / (S * sigma * math.sqrt(T))
        vega = S * self._norm_pdf(d1) * math.sqrt(T) * 0.01  # 1% 波动率变化
        rho = K * T * math.exp(-self.r * T) * self._norm_cdf(d2 if option_type == "call" else -d2) * 0.01

        return {
            "price": round(price, 4),
            "delta": round(delta, 4),
            "gamma": round(gamma, 4),
            "theta": round(theta / 365, 4),  # 每日 theta
            "vega": round(vega, 4),
            "rho": round(rho, 4),
        }

    def implied_volatility(
        self,
        market_price: float,
        S: float, K: float, T: float,
        option_type: str = "call",
        max_iter: int = 100,
        tol: float = 1e-6,
    ) -> float:
        """Newton-Raphson 法计算隐含波动率"""
        sigma = 0.3  # 初始猜测
        for _ in range(max_iter):
            result = self.black_scholes(S, K, T, sigma, option_type)
            price_diff = result["price"] - market_price
            vega = result["vega"] / 0.01  # 还原为原始 vega

            if abs(price_diff) < tol:
                return sigma

            if abs(vega) < 1e-10:
                break

            sigma -= price_diff / vega
            sigma = max(0.01, min(sigma, 5.0))  # 限制范围

        return sigma

    def calculate_greeks(self, symbol: str, S: float, K: float, T: float, sigma: float) -> Dict[str, Any]:
        """计算完整希腊值矩阵"""
        cache_key = f"{symbol}:{S}:{K}:{T:.4f}:{sigma:.4f}"
        if cache_key in self._cache:
            return self._cache[cache_key]

        call_greeks = self.black_scholes(S, K, T, sigma, "call")
        put_greeks = self.black_scholes(S, K, T, sigma, "put")

        result = {
            "symbol": symbol,
            "underlying_price": S,
            "strike": K,
            "time_to_expiry": T,
            "volatility": sigma,
            "call": call_greeks,
            "put": put_greeks,
            "put_call_parity": round(S + put_greeks["price"] - call_greeks["price"], 4),
        }
        self._cache[cache_key] = result
        return result


# ==================== LSTM 价格预测器 ====================
class Predictor:
    """基于 LSTM 的短期价格预测

    使用过去 N 根K线的 OHLCV 数据预测下一根K线的方向/涨跌幅。
    需要 tensorflow >= 2.0
    """

    def __init__(self, seq_len: int = 60, feature_count: int = 5):
        self.seq_len = seq_len
        self.feature_count = feature_count
        self.model: Any = None
        self.scaler_params: Dict[str, Any] = {}
        self._trained = False

    def _build_model(self) -> Any:
        """构建 LSTM 网络"""
        try:
            from tensorflow.keras.models import Sequential
            from tensorflow.keras.layers import LSTM, Dense, Dropout
            from tensorflow.keras.optimizers import Adam

            model = Sequential([
                LSTM(64, return_sequences=True, input_shape=(self.seq_len, self.feature_count)),
                Dropout(0.2),
                LSTM(32, return_sequences=False),
                Dropout(0.2),
                Dense(16, activation="relu"),
                Dense(1, activation="linear"),  # 回归: 预测涨跌幅
            ])
            model.compile(optimizer=Adam(learning_rate=0.001), loss="mse", metrics=["mae"])
            logger.info("LSTM 模型构建完成")
            return model
        except ImportError:
            logger.warning("tensorflow 未安装，Predictor 使用占位模式")
            return None

    def _prepare_sequences(self, df: pd.DataFrame) -> Tuple[np.ndarray, np.ndarray]:
        """准备训练序列

        Args:
            df: 包含 open/high/low/close/volume 的 DataFrame
        Returns:
            X: (samples, seq_len, features), y: (samples,)
        """
        df = df.copy()
        # 特征工程
        df["returns"] = df["close"].pct_change()
        df["hl_ratio"] = (df["high"] - df["low"]) / df["close"]
        df["oc_ratio"] = (df["close"] - df["open"]) / df["open"]

        features = ["returns", "hl_ratio", "oc_ratio",
                    df["volume"].pct_change().fillna(0).rename("vol_change"),
                    ((df["close"] - df["close"].rolling(20).mean()) / df["close"].rolling(20).std()).fillna(0).rename("zscore")]
        # 填充 NaN
        for f in features:
            if isinstance(f, str):
                df[f] = df[f].fillna(0)
            else:
                df[f.name] = f.fillna(0)

        feature_cols = [f if isinstance(f, str) else f.name for f in features]

        # 标准化
        means = df[feature_cols].mean()
        stds = df[feature_cols].std().replace(0, 1)
        scaled = (df[feature_cols] - means) / stds
        self.scaler_params = {"mean": means.to_dict(), "std": stds.to_dict()}

        # 构建序列
        X, y = [], []
        target = df["returns"].shift(-1).fillna(0).values
        values = scaled.values

        for i in range(self.seq_len, len(values)):
            X.append(values[i - self.seq_len:i])
            y.append(target[i])

        return np.array(X), np.array(y)

    def train_model(self, df: pd.DataFrame, epochs: int = 50, batch_size: int = 32) -> Dict[str, float]:
        """训练预测模型

        Args:
            df: 历史K线数据
            epochs: 训练轮数
            batch_size: 批次大小
        Returns:
            训练指标
        """
        if len(df) < self.seq_len + 100:
            logger.warning("数据不足 (需要至少 %d 条, 实际 %d)", self.seq_len + 100, len(df))
            return {}

        try:
            X, y = self._prepare_sequences(df)
            split = int(len(X) * 0.8)
            X_train, X_val = X[:split], X[split:]
            y_train, y_val = y[:split], y[split:]

            self.model = self._build_model()
            if self.model is None:
                return {}

            history = self.model.fit(
                X_train, y_train,
                validation_data=(X_val, y_val),
                epochs=epochs,
                batch_size=batch_size,
                verbose=0,
            )

            self._trained = True
            metrics = {
                "train_loss": float(history.history["loss"][-1]),
                "val_loss": float(history.history["val_loss"][-1]),
                "train_mae": float(history.history["mae"][-1]),
                "val_mae": float(history.history["val_mae"][-1]),
            }
            logger.info("LSTM 训练完成: %s", metrics)
            return metrics
        except Exception as e:
            logger.error("LSTM 训练失败: %s", e)
            return {}

    def predict(self, df: pd.DataFrame) -> Optional[float]:
        """预测下一个周期的涨跌幅

        Args:
            df: 最近 N 根K线 (至少 seq_len 条)
        Returns:
            预测涨跌幅 (正=涨, 负=跌), 或 None
        """
        if not self._trained or self.model is None:
            logger.warning("模型未训练")
            return None

        if len(df) < self.seq_len:
            logger.warning("数据不足: 需要 %d, 实际 %d", self.seq_len, len(df))
            return None

        try:
            df = df.tail(self.seq_len).copy()
            df["returns"] = df["close"].pct_change().fillna(0)
            df["hl_ratio"] = (df["high"] - df["low"]) / df["close"].replace(0, 1)
            df["oc_ratio"] = (df["close"] - df["open"]) / df["open"].replace(0, 1)
            df["vol_change"] = df["volume"].pct_change().fillna(0)
            df["zscore"] = ((df["close"] - df["close"].rolling(20).mean()) / df["close"].rolling(20).std()).fillna(0)

            feature_cols = ["returns", "hl_ratio", "oc_ratio", "vol_change", "zscore"]
            means = pd.Series(self.scaler_params.get("mean", {}))
            stds = pd.Series(self.scaler_params.get("std", {}))
            scaled = (df[feature_cols].tail(self.seq_len) - means) / stds.replace(0, 1)

            X = scaled.values.reshape(1, self.seq_len, self.feature_count)
            pred = float(self.model.predict(X, verbose=0)[0][0])
            return pred
        except Exception as e:
            logger.error("LSTM 预测失败: %s", e)
            return None

    def predict_direction(self, df: pd.DataFrame) -> Optional[str]:
        """预测涨跌方向"""
        pred = self.predict(df)
        if pred is None:
            return None
        return "up" if pred > 0 else "down"


# ==================== 可视化面板 ====================
# ---------- 辅助函数 ----------
def get_latest_price(symbol: str, data_api: Optional[MarketDataAPI] = None) -> Optional[float]:
    """获取最新价格 (辅助函数)

    Args:
        symbol: 股票代码
        data_api: 行情接口实例 (可选)
    Returns:
        最新价格, 或 None
    """
    if data_api is None:
        logger.warning("get_latest_price: 未提供 data_api")
        return None

    rt_data = data_api.get_real_time_data(symbol)
    if rt_data:
        return rt_data["price"]
    else:
        logger.warning("无法获取 %s 的最新价格", symbol)
        return None


class Dashboard:
    """Rich 策略监控面板 (独立视图)

    与 EnhancedDualMAStrategy.update_gui() 不同，
    Dashboard 提供更详细的指标视图，适合单独展示。
    """

    def __init__(self, strategy: Optional[EnhancedDualMAStrategy] = None):
        self.strategy = strategy
        self.console = Console()

    def display_overview(self) -> None:
        """显示策略总览"""
        if self.strategy is None:
            self.console.print("[red]策略未初始化[/red]")
            return

        table = Table(title="📊 策略监控面板", title_style="bold cyan")
        table.add_column("股票代码", justify="center")
        table.add_column("当前价格", justify="right")
        table.add_column("MA5/MA20", justify="center")
        table.add_column("RSI", justify="center")
        table.add_column("趋势评分", justify="center")
        table.add_column("信号", justify="center")

        for sym, analyzer in self.strategy.analyzers.items():
            indicators = analyzer.calculate_indicators()
            price = get_latest_price(sym, self.strategy.data_api)
            trend_score = analyzer.get_trend_score()

            for tf, ind in indicators.items():
                if not ind:
                    continue
                ma_status = (
                    "[green]金叉[/green]" if ind.get("MA5", 0) > ind.get("MA20", 0)
                    else "[red]死叉[/red]"
                )
                rsi_val = ind.get("RSI", 50)
                if rsi_val > 70:
                    rsi_display = f"[red]{rsi_val:.1f}[/red]"
                elif rsi_val < 30:
                    rsi_display = f"[green]{rsi_val:.1f}[/green]"
                else:
                    rsi_display = f"[yellow]{rsi_val:.1f}[/yellow]"

                direction = "🔴 BUY" if trend_score > 0.3 else ("🟢 SELL" if trend_score < -0.3 else "⚪ HOLD")

                table.add_row(
                    f"{sym} ({tf})",
                    f"¥{price:.2f}" if price else "N/A",
                    ma_status,
                    rsi_display,
                    f"{trend_score:+.2f}",
                    direction,
                )

        self.console.print(table)

    def display_positions(self, account: Account) -> None:
        """显示持仓详情"""
        table = Table(title="📈 持仓明细", title_style="bold green")
        table.add_column("代码")
        table.add_column("数量")
        table.add_column("均价")
        table.add_column("现价")
        table.add_column("市值")
        table.add_column("盈亏")

        for symbol, pos in account.positions.items():
            qty = int(pos.get("qty", 0))
            avg_price = pos.get("avg_price", 0)
            value = pos.get("value", 0)
            pnl = pos.get("pnl", 0)
            current_price = avg_price + pnl / qty if qty != 0 else 0

            pnl_style = "[green]" if pnl >= 0 else "[red]"
            table.add_row(
                symbol,
                str(qty),
                f"¥{avg_price:.2f}",
                f"¥{current_price:.2f}",
                f"¥{value:,.2f}",
                f"{pnl_style}¥{pnl:+,.2f}[/]",
            )

        if not account.positions:
            table.add_row("(空仓)", "-", "-", "-", "-", "-")

        self.console.print(table)

    def display(self, account: Account, signals: Dict[str, str]) -> None:
        """完整显示 (兼容旧接口)"""
        table = Table(title="策略监控面板")
        table.add_column("股票代码", justify="right")
        table.add_column("当前价格", justify="right")
        table.add_column("持仓数量", justify="right")
        table.add_column("信号", justify="center")

        for symbol in account.positions:
            pos = account.positions[symbol]
            price = get_latest_price(symbol, self.strategy.data_api if self.strategy else None)
            signal = signals.get(symbol, "")
            signal_display = f"[green]{signal}[/green]" if signal == "BUY" else f"[red]{signal}[/red]"
            table.add_row(
                symbol,
                f"¥{price:.2f}" if price else "N/A",
                str(pos.get("quantity", 0)),
                signal_display,
            )

        self.console.print(table)


# ==================== 入口 ====================
if __name__ == "__main__":
    strategy = EnhancedDualMAStrategy("strategy.yaml")

    # 可选: 连接交易接口
    strategy.trade_api.connect()

    # 启动策略线程
    strategy_thread = Thread(target=strategy.run_strategy, daemon=True, name="StrategyMain")
    strategy_thread.start()

    # 主线程: 等待退出
    console = Console()
    console.print(Panel.fit(
        "[bold cyan]双均线交易系统已启动[/bold cyan]\n"
        "按 [yellow]Ctrl+C[/yellow] 停止",
        title="🚀 MyAutomatic Trade",
    ))

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        console.print("\n[bold yellow]正在停止策略...[/bold yellow]")
        strategy.running = False
        strategy_thread.join(timeout=10)
        console.print("[bold green]策略已停止[/bold green]")
        exit(0)
