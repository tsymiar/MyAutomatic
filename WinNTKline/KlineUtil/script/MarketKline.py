#!/usr/bin/python
# coding: utf-8
# from matplotlib.finance import quotes_historical_yahoo_ochl, candlestick_ohlc
from matplotlib.dates import (
    DateFormatter,
    WeekdayLocator,
    DayLocator,
    MONDAY,
    ## YEARLY
)

# import matplotlib.figure as fig
# import matplotlib.mlab as mlab
import matplotlib.pyplot as plt
import numpy as np
import importlib

# import pylab
# import os
import time
import sys
import six

fileEncoding = "gb2312"

if sys.getdefaultencoding() != fileEncoding:
    importlib.reload(sys)


def main():
    plt.rcParams["font.sans-serif"] = ["SimHei"]
    plt.rcParams["axes.unicode_minus"] = False

    ticker = "600747"  # 600747 是"大连控股"的股票代码
    ticker += ".ss"  # .ss 表示上证 .sz表示深证

    date1 = (2007, 1, 1)  # 起始日期，格式：(年，月，日)元组
    date2 = (2011, 1, 1)  # 结束日期，格式：(年，月，日)元组

    weekdays = WeekdayLocator(MONDAY)  # 主要刻度
    allday = DayLocator()  # 次要刻度
    # mondayFormatter = DateFormatter("%b %d")  # eg.: Jan 12
    alldayFormatter = DateFormatter("%Y-%m-%d")  # eg.: 2-29-2015
    dayFormatter = DateFormatter("%d")  # eg.: 12

    quotes = quotes_historical_yahoo_ochl(ticker, date1, date2)

    if len(quotes) == 0:
        raise SystemExit

    fig, ax = plt.subplots()
    fig.subplots_adjust(bottom=0.2)

    ax.xaxis.set_major_locator(weekdays)
    ax.xaxis.set_minor_locator(allday)
    ax.xaxis.set_major_formatter(alldayFormatter)
    ax.xaxis.set_minor_formatter(dayFormatter)

    plot_day_summary(ax, quotes, ticksize=3)
    candlestick(ax, quotes, width=0.6, colorup="red", colordown="green")

    ax.xaxis_date()
    ax.autoscale_view()
    plt.setp(plt.gca().get_xticklabels(), rotation=45, horizontalalignment="right")

    ax.grid(True)
    plt.title("600747")
    plt.show()
    return


# 从文件读取数据并返回数据列表
def loadPoints(file):
    filename = open(file, "r", encoding=fileEncoding)
    amount = []
    price = []
    openVal = []
    close = []
    high = []
    date = []
    low = []
    points = []
    for count, line in enumerate(filename):
        values = line.strip("\n")
        values = line.split("\t")  # 按TAB把数据分开
        if count <= 1:
            values = line.split(" ")
            print("stock: " + values[0] + ", " + values[1] + ", " + values[2])
        else:
            values = line.split("\t")
            date.append(values[0])
            openVal.append(float(values[1]))
            high.append(float(values[2]))
            low.append(float(values[3]))
            close.append(float(values[4]))
            amount.append(int(values[5]))
            price.append(float(values[6]))
        points = [date, amount, openVal, close, high, low, price]
    return points


# 截取
def txt_wrap_by(head, tail, text):
    start = text.find(head)
    if start >= 0:
        start += len(head)
        tail = text.find(tail, start)
        if tail >= 0:
            return text[start:tail].strip()


# 绘制图表
def figureOut(m):
    for idx, clr in enumerate("yc"):
        sub = plt.subplot(211 + idx)
        sub.set_facecolor(clr)
    _type_ = [
        str(txt_wrap_by("(", ")", six.text_type(m[4][0]))),
        str(txt_wrap_by("(", ")", six.text_type(m[4][1]))),
    ]
    h = []
    n = len(m[0])  # 横轴长度
    mxt = max(m[2])
    mnt = min(m[2])
    x = np.linspace(0, n, n)  # 横轴坐标值
    if max(m[3]) >= mxt:
        mxt = max(m[3])
    if min(m[3]) <= mnt:
        mnt = min(m[3])
    if max(m[4]) >= mxt:
        mxt = max(m[4])
    if min(m[4]) <= mnt:
        mnt = min(m[4])
    if max(m[5]) >= mxt:
        mxt = max(m[5])
    if min(m[5]) <= mnt:
        mnt = min(m[5])
    for i in range(len(_type_) - 1):
        h.append(plt.figure(figsize=(12, 6)))
    ax = plt.subplot(111)
    # 自定义坐标值
    ax.set_xticks(range(1, n, 1))
    ax.set_xticklabels(
        ["%d" % val for val in range(1, n, 1)], rotation=10, fontsize=3, color="blue"
    )
    # 纵轴
    plt.ylabel("Volt")
    plt.ylim(mnt - 1, mxt + 1)
    # 图例
    plt.plot(x, m[2], label="$open$", color="green", linewidth=1)
    plt.plot(x, m[3], label="$close$", color="yellow")
    plt.plot(x, m[4], "b--", color="red", label="$high$")
    plt.plot(x, m[5], "b--", label="$low$")
    plt.xlabel("NO.")
    plt.legend(loc="upper left", bbox_to_anchor=(0.9, 0.4))
    # 填充数据
    for i, (_x, _y) in enumerate(zip(x, m[0])):
        if m[3][i] != m[3][i - 1]:
            plt.text(_x, mxt + 5, int(m[3][i]), color="c", fontsize=10)
    plt.text(0.5, 0.92, _type_[0], color="green", fontsize=10)
    plt.title(_type_[0] + "-TPS")
    # 网格
    plt.grid(True)
    plt.text(
        0.9, 0.4, "connect number", color="cyan", ha="center", transform=ax.transAxes
    )
    plt.annotate(
        "bottom",
        xy=(0, 0),
        xytext=(0.2, 0.2),
        arrowprops=dict(facecolor="blue", shrink=0.05),
    )
    currentTime = str(time.strftime("%Y%m%d-%H%M%S", time.localtime(time.time())))
    hint = [
        "[TPS]",
        str(_type_[0]),
        "_",
        str(int(min(m[3]))),
        "-",
        str(int(max(m[3]))),
        "_",
        currentTime,
        ".png",
    ]
    # plt.savefig("".join(hint), dpi=480)
    print(hint)
    plt.show()


if __name__ == "__main__":
    for param in sys.argv:
        if not param.strip():
            print(param)
    param = "../data/SH600747.DAT"
    figureOut(loadPoints(param))
    input("Press <Enter> to exit:")
    main()
