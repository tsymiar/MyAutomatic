#!/usr/bin/python
# coding: utf-8

"""
股票双均线交易系统专业版
支持：
1.真实行情接入、
2.券商接口支持、
3.周期分析引擎、
4.风险控制模块
"""

# graph TD
#     A[行情网关] --> B(策略引擎)
#     B --> C{风控引擎}
#     C --> D[交易网关]
#     D --> E[券商系统]
#     B --> F[监控中心]
#     F --> G[Web界面]
#     F --> H[移动报警]
#     E --> I((清算系统))

# 安装依赖
# pip install -r requirements.txt
# 查看日志
# tail -f logs/strategy.log

import pandas as pd

# import numpy as np
import time
import requests

# from datetime import datetime
from threading import Thread, Lock
import talib
from abc import ABC, abstractmethod
from rich.console import Console
from rich.table import Table
from rich.layout import Layout
import yaml
import os

# import hashlib
import json
import websocket
import logging


# ------------------ 行情接口抽象类 ------------------
class MarketDataAPI(ABC):
    @abstractmethod
    def get_real_time_data(self, symbol):
        pass

    @abstractmethod
    def get_historical_data(self, symbol, timeframe):
        pass


# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
)


# ------------------ Tushare行情实现 ------------------
class TushareAPI(MarketDataAPI):
    def __init__(self, token):
        self.token = token
        self.base_url = "http://api.tushare.pro"

    def _request(self, api_name, **params):
        data = {
            "api_name": api_name,
            "token": self.token,
            "params": params,
            "fields": "",
        }
        resp = requests.post(self.base_url, json=data)
        try:
            return resp.json()
        except Exception as e:
            logging.error("JSON解析错误: %s", e)
            return None

    def get_real_time_data(self, symbol):
        data = self._request("realtime_quote", ts_code=symbol)
        if not data or "data" not in data:
            logging.error("实时行情数据为空: %s", symbol)
            return None
        try:
            item = data["data"]["items"][0]
        except (IndexError, KeyError, TypeError) as e:
            logging.error("解析实时行情数据失败: %s, error: %s", symbol, e)
            return None
        return {
            "symbol": symbol,
            "price": float(item[21]),
            "volume": int(item[13]),
        }

    def get_historical_data(self, symbol, timeframe):
        freq_map = {"15m": "15MIN", "30m": "30MIN"}
        json_rsp = self._request("pro_bar", ts_code=symbol, freq=freq_map[timeframe])
        if not json_rsp or "data" not in json_rsp:
            logging.error("获取历史数据失败: %s, timeframe: %s", symbol, timeframe)
            return pd.DataFrame()
        try:
            data = json_rsp["data"]
            if data == None:
                logging.error("解析历史数据失败: %s, msg: %s", symbol, json_rsp["msg"])
                return pd.DataFrame()
            items = data["items"]
        except (KeyError, TypeError) as e:
            logging.error("解析历史数据失败: %s, error: %s", symbol, e)
            return pd.DataFrame()
        return pd.DataFrame(items)


# ------------------ 交易接口抽象类 ------------------
class TradeAPI(ABC):
    @abstractmethod
    def connect(self):
        pass

    @abstractmethod
    def place_order(self, symbol, price, qty, direction):
        pass

    @abstractmethod
    def get_positions(self):
        pass


# ------------------ CTP交易接口实现 ------------------
class CTPTradeAPI(TradeAPI):
    def __init__(self, config):
        self.config = config
        self.ws = None
        self.connected = False

    def connect(self):
        def on_message(ws, message):
            logging.info("Received: %s", message)

        self.ws = websocket.WebSocketApp(
            self.config["trade_server"], on_message=on_message
        )
        Thread(target=self.ws.run_forever, daemon=True).start()
        time.sleep(3)  # 等待连接建立

    def place_order(self, symbol, price, qty, direction):
        order = {
            "Symbol": symbol,
            "Price": price,
            "Quantity": qty,
            "Direction": direction,
        }
        try:
            self.ws.send(json.dumps(order))
            logging.info("Order sent: %s", order)
        except Exception as e:
            logging.error("Failed to send order %s, error: %s", order, e)

    def get_positions(self):
        try:
            self.ws.send(json.dumps({"action": "query_position"}))
        except Exception as e:
            logging.error("Failed to query positions, error: %s", e)
        # 需要实现异步处理...


# ------------------ 周期分析引擎 ------------------
class MultiTimeframeAnalyzer:
    def __init__(self, symbol, timeframes, data_api):
        self.symbol = symbol
        self.timeframes = timeframes
        self.data_api = data_api
        self.data = {tf: pd.DataFrame() for tf in timeframes}

    def update_data(self):
        for tf in self.timeframes:
            new_data = self.data_api.get_historical_data(self.symbol, tf)
            self.data[tf] = pd.concat([self.data[tf], new_data]).tail(500)

    def calculate_indicators(self):
        indicators = {}
        for tf in self.timeframes:
            df = self.data[tf]
            # 计算多指标
            df["MA5"] = talib.SMA(df.close, 5)
            df["MA20"] = talib.SMA(df.close, 20)
            df["RSI"] = talib.RSI(df.close, 14)
            df["MACD"], _, _ = talib.MACD(df.close)
            indicators[tf] = df.iloc[-1].to_dict()
        return indicators


# ------------------ 高级风控模块 ------------------
class RiskManager:
    def __init__(self, config):
        self.config = config
        self.position_lock = Lock()

    def check_order(self, order, account):
        with self.position_lock:
            # 检查单日最大亏损
            if account.daily_pnl < self.config["daily_loss_limit"]:
                return False, "Exceed daily loss limit"

            # 检查杠杆率
            total_value = account.total_assets
            position_value = sum(p["value"] for p in account.positions.values())
            leverage = position_value / total_value
            if leverage > self.config["max_leverage"]:
                return False, "Exceed max leverage"

            # 检查单品种仓位
            symbol = order["symbol"]
            position_ratio = account.positions.get(symbol, 0) / total_value
            if position_ratio > self.config["max_position_ratio"]:
                return False, "Exceed position ratio"

            return True, "Approved"


# ------------------ 策略核心类 ------------------
class EnhancedDualMAStrategy:
    def __init__(self, config_file):
        if not os.path.exists(config_file):
            logging.error("配置文件不存在: %s", config_file)
            exit(1)
        self.load_config(config_file)
        self.init_components()
        self.running = True

    def load_config(self, file_path):
        with open(file_path) as f:
            self.config = yaml.safe_load(f)

    def init_components(self):
        # 初始化行情接口
        if self.config["market"]["data_source"] == "tushare":
            self.data_api = TushareAPI(self.config["market"]["tushare_token"])

        # 初始化交易接口
        if self.config["account"]["broker"] == "ctp":
            self.trade_api = CTPTradeAPI(self.config["account"])

        # 初始化分析引擎
        self.analyzers = {
            sym: MultiTimeframeAnalyzer(
                sym, self.config["strategy"]["timeframes"], self.data_api
            )
            for sym in self.config["strategy"]["symbols"]
        }

        # 初始化风控
        self.risk_mgr = RiskManager(self.config["risk"])

        # 初始化账户
        self.account = {"total_assets": 1_000_000, "positions": {}, "daily_pnl": 0}

    def run_strategy(self):
        while self.running:
            try:
                # 更新行情数据
                for sym in self.config["strategy"]["symbols"]:
                    self.analyzers[sym].update_data()

                # 生成交易信号
                signals = self.generate_signals()

                # 执行风控检查
                valid_orders = []
                for order in signals:
                    is_valid, reason = self.risk_mgr.check_order(order, self.account)
                    if is_valid:
                        valid_orders.append(order)
                    else:
                        logging.warning("Order rejected: %s, reason: %s", order, reason)

                # 执行交易
                self.execute_orders(valid_orders)

                # 更新监控界面
                self.update_gui()

                time.sleep(5)

            except Exception as e:
                logging.error("策略执行异常: %s", e)

    def generate_signals(self):
        signals = []
        for sym, analyzer in self.analyzers.items():
            indicators = analyzer.calculate_indicators()
            bullish = all(ind["MA5"] > ind["MA20"] for ind in indicators.values())
            bearish = all(ind["MA5"] < ind["MA20"] for ind in indicators.values())
            rt_data = self.data_api.get_real_time_data(sym)
            if rt_data is None:
                # 跳过此 symbol
                continue

            if bullish:
                signals.append(
                    {
                        "symbol": sym,
                        "direction": "buy",
                        "price": rt_data["price"],
                    }
                )
            elif bearish:
                signals.append(
                    {
                        "symbol": sym,
                        "direction": "sell",
                        "price": rt_data["price"],
                    }
                )
        return signals

    def execute_orders(self, orders):
        for order in orders:
            self.trade_api.place_order(
                symbol=order["symbol"],
                price=order["price"],
                qty=self.calculate_position_size(order),
                direction=order["direction"],
            )

    def calculate_position_size(self, order):
        # 基于波动率的仓位计算
        symbol = order["symbol"]
        recent_data = self.analyzers[symbol].data["15m"]
        atr = talib.ATR(recent_data.high, recent_data.low, recent_data.close, 14)[-1]
        risk_capital = self.account["total_assets"] * 0.01  # 1%风险资本
        return int(risk_capital / atr)

    def update_gui(self):
        console = Console()
        layout = Layout()
        layout.split(
            Layout(name="header", size=3),
            Layout(name="main", ratio=1),
            Layout(name="footer", size=7),
        )

        # 头部
        header_table = Table(title="账户概览")
        header_table.add_column("总资产")
        header_table.add_column("可用资金")
        header_table.add_column("当日盈亏")
        header_table.add_row(
            f"{self.account['total_assets']:,.2f}",
            f"{self.account['total_assets'] - sum(p['value'] for p in self.account['positions'].values()):,.2f}",
            f"{self.account['daily_pnl']:+,.2f}",
        )
        layout["header"].update(header_table)

        # 主体
        main_table = Table(title="持仓监控")
        main_table.add_column("代码")
        main_table.add_column("方向")
        main_table.add_column("数量")
        main_table.add_column("市值")
        main_table.add_column("盈亏")
        for sym, pos in self.account["positions"].items():
            main_table.add_row(
                sym,
                "多" if pos["qty"] > 0 else "空",
                str(abs(pos["qty"])),
                f"{pos['value']:,.2f}",
                f"{pos['pnl']:+,.2f}",
            )
        layout["main"].update(main_table)

        console.print(layout)


# ------------------ 优化措施 ------------------

# ---------- 回测模块 ----------
from backtrader import Cerebro


class BacktestEngine:
    def run_backtest(self):
        bro = Cerebro()
        bro.addstrategy(EnhancedDualMAStrategy)
        bro.run()


# 启用异步IO
import asyncio


async def data_updater(self):
    while self.running:
        await self.update_market_data()
        await asyncio.sleep(1)


class HTSCAPI(TradeAPI):
    """华泰证券接口实现"""

    def connect(self):
        # 实现华泰特有的连接协议
        pass


class DerivativesEngine:
    def calculate_greeks(self):
        """期权希腊值计算"""


class Predictor:
    def train_model(self, data):
        # 使用LSTM进行价格预测
        from tensorflow.keras.models import Sequential

        model = Sequential()
        Console().print(model)
        # ...神经网络结构定义


# ---------- 可视化面板 ----------


def get_latest_price(symbol):
    """
    获取最新的股票价格
    :param symbol: 股票代码
    :return: 最新价格
    """
    # 假设使用 TushareAPI 获取实时数据
    tushare_api = TushareAPI(
        token=EnhancedDualMAStrategy.config["market"]["tushare_token"]
    )
    real_time_data = tushare_api.get_real_time_data(symbol)
    if real_time_data:
        return real_time_data["price"]
    else:
        logging.error("无法获取股票 %s 的最新价格", symbol)
        return None


class Dashboard:
    def display(self, account, signals):
        console = Console()
        table = Table(title="策略监控面板")

        # 添加列
        table.add_column("股票代码", justify="right")
        table.add_column("当前价格", justify="right")
        table.add_column("持仓数量", justify="right")
        table.add_column("信号", justify="center")

        # 添加行
        for symbol in account.positions:
            pos = account.positions[symbol]
            price = get_latest_price(symbol)
            signal = signals.get(symbol, "")
            table.add_row(
                symbol,
                f"{price:.2f}",
                str(pos["quantity"]),
                f"[green]{signal}" if signal == "BUY" else f"[red]{signal}",
            )

        console.print(table)


# ------------------ 示例 ------------------
if __name__ == "__main__":
    strategy = EnhancedDualMAStrategy("strategy.yaml")

    # 启动策略线程
    strategy_thread = Thread(target=strategy.run_strategy)
    strategy_thread.start()

    # 主线程循环
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        strategy.running = False
        strategy_thread.join()
        Console().print("策略已停止")
        exit(0)
