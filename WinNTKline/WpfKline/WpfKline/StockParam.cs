using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace WpfKline
{
    /// <summary>
    /// 股票信息
    /// </summary>
    public class StockParam
    {
        /// <summary>
        /// 时间
        /// </summary>
        public DateTime date { get; set; }

        /// <summary>
        /// 开盘价
        /// </summary>
        public double open { get; set; }
        /// <summary>
        /// 最高价
        /// </summary>
        public double high { get; set; }
        /// <summary>
        /// 最低价
        /// </summary>
        public double low { get; set; }
        /// <summary>
        /// 收盘价
        /// </summary>
        public double close { get; set; }

        /// <summary>
        /// 成交量
        /// </summary>
        public double volume { get; set; }
    }
}
