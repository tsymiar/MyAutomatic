using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace WPFKline
{
    public partial class MainWindow : Window
    {
        private string stockName = string.Empty;
        private bool _tooltipActive;
        private bool _mouseOverTooltip;

        public List<StockParam> Data { get; set; }
        private List<StockParam> _allData;            // 完整数据，不受日期过滤影响

        public MainWindow()
        {
            InitializeComponent();
        }

        // 数据加载

        private List<StockParam> LoadStockDetail(string fileName)
        {
            if (!File.Exists(fileName))
            {
                MessageBox.Show(
                    "文件不存在: " + Path.GetFileName(fileName), "错误",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return null;
            }

            try
            {
                using (var reader = new StreamReader(fileName, Encoding.GetEncoding("GB2312")))
                {
                    var lines = reader.ReadToEnd()
                        .Split(new[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);

                    if (lines.Length < 3)
                    {
                        MessageBox.Show("数据文件格式不正确（至少需要3行）", "错误");
                        return null;
                    }

                    stockName = lines[0].Replace("\r", "").Trim();
                    var result = new List<StockParam>(lines.Length - 2);

                    for (int i = 2; i < lines.Length; i++)
                    {
                        string[] cols = lines[i].Split('\t');
                        if (cols.Length < 6) continue;

                        result.Add(new StockParam
                        {
                            date   = DateTime.Parse(cols[0]),
                            open   = double.Parse(cols[1]),
                            high   = double.Parse(cols[2]),
                            low    = double.Parse(cols[3]),
                            close  = double.Parse(cols[4]),
                            volume = double.Parse(cols[5])
                        });
                    }
                    return result;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(
                    "加载文件失败: " + ex.Message, "错误",
                    MessageBoxButton.OK, MessageBoxImage.Error);
                return null;
            }
        }

        private void LoadFile()
        {
            string rootPath = System.Windows.Forms.Application.StartupPath;
            if (string.IsNullOrEmpty(rootPath)) return;

            string defaultFile = rootPath +
#if DEBUG
                @"\..\..\KlineUtil\data\SH600747.DAT";
#else
                @"\data\SH600747.DAT";
#endif
            txtFilePath.Text = defaultFile;
            LoadAndBindData(defaultFile);
        }

        private void LoadAndBindData(string path)
        {
            Data = LoadStockDetail(path);
            if (Data == null)
            {
                tbStatus.Text = "加载失败";
                return;
            }

            _allData = new List<StockParam>(Data);

            stockSet.ItemsSource = Data;

            tbStockTitle.Text = string.IsNullOrEmpty(stockName) ? "未命名股票" : stockName;
            tbStockSubtitle.Text = Path.GetFileName(path);
            tbStatus.Text = "已加载";
            tbRecordCount.Text = string.Format("共 {0:N0} 条记录", Data.Count);

            if (stockChart.Charts.Count > 0)
                stockChart.Charts[0].Graphs[0].Title = stockName;
            if (stockChart.Charts.Count > 1)
                stockChart.Charts[1].Graphs[0].Title = stockName;

            SyncCalendarRange();
        }

        // 窗口事件

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            DataContext = this;
            LoadFile();
        }

        private void btnOpenFile_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.OpenFileDialog
            {
                Filter = "数据文件(*.DAT)|*.DAT|所有文件(*.*)|*.*",
                RestoreDirectory = true
            };
            if (dlg.ShowDialog() == true && !string.IsNullOrEmpty(dlg.FileName))
            {
                txtFilePath.Text = dlg.FileName;
                LoadAndBindData(dlg.FileName);
            }
        }

        private void txtFilePath_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Return)
            {
                LoadAndBindData(txtFilePath.Text);
                e.Handled = true;
            }
        }

        // 日期范围（手写迷你日历，兼容 .NET 3.5）

        private int _calStartYear, _calStartMonth, _calEndYear, _calEndMonth;
        private DateTime _selectedStartDate, _selectedEndDate;

        private void btnStartDate_Click(object sender, RoutedEventArgs e)
        {
            popStartDate.IsOpen = true;
        }

        private void btnEndDate_Click(object sender, RoutedEventArgs e)
        {
            popEndDate.IsOpen = true;
        }

        private void SyncCalendarRange()
        {
            if (_allData == null || _allData.Count == 0) return;

            DateTime min = _allData[0].date;
            DateTime max = _allData[_allData.Count - 1].date;

            SetCalendarDate(true, min);
            SetCalendarDate(false, max);
        }

        private void SetCalendarDate(bool isStart, DateTime dt)
        {
            if (isStart)
            {
                _selectedStartDate = dt;
                _calStartYear = dt.Year;
                _calStartMonth = dt.Month;
                btnStartDate.Content = dt.ToString("yyyy-MM-dd");
                RebuildDayGrid(true);
            }
            else
            {
                _selectedEndDate = dt;
                _calEndYear = dt.Year;
                _calEndMonth = dt.Month;
                btnEndDate.Content = dt.ToString("yyyy-MM-dd");
                RebuildDayGrid(false);
            }
        }

        // 日历导航

        private void calStart_PrevYear(object sender, RoutedEventArgs e)  { _calStartYear--;  RebuildDayGrid(true); }
        private void calStart_NextYear(object sender, RoutedEventArgs e)  { _calStartYear++;  RebuildDayGrid(true); }
        private void calStart_PrevMonth(object sender, RoutedEventArgs e) { if (--_calStartMonth < 1)  { _calStartMonth = 12; _calStartYear--; } RebuildDayGrid(true); }
        private void calStart_NextMonth(object sender, RoutedEventArgs e) { if (++_calStartMonth > 12) { _calStartMonth = 1;  _calStartYear++; } RebuildDayGrid(true); }

        private void calEnd_PrevYear(object sender, RoutedEventArgs e)   { _calEndYear--;  RebuildDayGrid(false); }
        private void calEnd_NextYear(object sender, RoutedEventArgs e)   { _calEndYear++;  RebuildDayGrid(false); }
        private void calEnd_PrevMonth(object sender, RoutedEventArgs e)  { if (--_calEndMonth < 1)  { _calEndMonth = 12; _calEndYear--; } RebuildDayGrid(false); }
        private void calEnd_NextMonth(object sender, RoutedEventArgs e)  { if (++_calEndMonth > 12) { _calEndMonth = 1;  _calEndYear++; } RebuildDayGrid(false); }

        // 重建日期格子

        private void RebuildDayGrid(bool isStart)
        {
            Grid grid = isStart ? gridStartDays : gridEndDays;
            TextBlock header = isStart ? lblStartHeader : lblEndHeader;
            int year = isStart ? _calStartYear : _calEndYear;
            int month = isStart ? _calStartMonth : _calEndMonth;
            DateTime selected = isStart ? _selectedStartDate : _selectedEndDate;

            header.Text = string.Format("{0}年{1}月", year, month);
            grid.Children.Clear();
            grid.RowDefinitions.Clear();
            grid.ColumnDefinitions.Clear();

            // 7列，周一~周日
            for (int c = 0; c < 7; c++)
                grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(26) });

            DateTime firstDay = new DateTime(year, month, 1);
            int startCol = ((int)firstDay.DayOfWeek + 6) % 7;
            int daysInMonth = DateTime.DaysInMonth(year, month);
            int totalCells = startCol + daysInMonth;
            int rows = (totalCells + 6) / 7;

            for (int r = 0; r < Math.Max(rows, 6); r++)
                grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(22) });

            for (int i = 0; i < startCol + daysInMonth; i++)
            {
                int row = i / 7;
                int col = i % 7;

                if (i < startCol) continue;

                int day = i - startCol + 1;
                bool isToday = (day == selected.Day && month == selected.Month && year == selected.Year);

                Button btn = new Button
                {
                    Content = day.ToString(),
                    Width = 24, Height = 20,
                    FontSize = 10, Padding = new Thickness(0),
                    Margin = new Thickness(1),
                    Cursor = Cursors.Hand,
                    Background = isToday
                        ? new SolidColorBrush(Color.FromRgb(0x33, 0x99, 0xFF))
                        : Brushes.White,
                    Foreground = isToday ? Brushes.White : Brushes.Black,
                    BorderBrush = isToday
                        ? new SolidColorBrush(Color.FromRgb(0x33, 0x99, 0xFF))
                        : Brushes.Transparent,
                };

                int capturedYear = year, capturedMonth = month, capturedDay = day;
                bool capturedIsStart = isStart;
                btn.Click += (s, ev) =>
                {
                    DateTime chosen = new DateTime(capturedYear, capturedMonth, capturedDay);
                    if (capturedIsStart)
                    {
                        _selectedStartDate = chosen;
                        _calStartYear = capturedYear;
                        _calStartMonth = capturedMonth;
                        btnStartDate.Content = chosen.ToString("yyyy-MM-dd");
                        RebuildDayGrid(true);
                        popStartDate.IsOpen = false;
                    }
                    else
                    {
                        _selectedEndDate = chosen;
                        _calEndYear = capturedYear;
                        _calEndMonth = capturedMonth;
                        btnEndDate.Content = chosen.ToString("yyyy-MM-dd");
                        RebuildDayGrid(false);
                        popEndDate.IsOpen = false;
                    }
                };

                Grid.SetRow(btn, row);
                Grid.SetColumn(btn, col);
                grid.Children.Add(btn);
            }
        }

        // 应用 / 重置

        private void btnApplyDate_Click(object sender, RoutedEventArgs e)
        {
            if (_allData == null || _allData.Count == 0)
            {
                MessageBox.Show("请先加载数据文件", "提示",
                    MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }

            DateTime start = _selectedStartDate, end = _selectedEndDate;
            if (start == default || end == default)
            {
                MessageBox.Show("请先通过日历选择起止日期", "提示",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            if (start > end)
            {
                MessageBox.Show("起始日期不能晚于结束日期", "提示",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            Data = _allData.Where(d => d.date >= start && d.date <= end).ToList();
            stockSet.ItemsSource = Data;
            tbRecordCount.Text = string.Format("共 {0:N0} 条记录（已过滤）", Data.Count);
            tbStatus.Text = string.Format("时段: {0:yyyy-MM-dd} ~ {1:yyyy-MM-dd}", start, end);
        }

        private void btnResetDate_Click(object sender, RoutedEventArgs e)
        {
            if (_allData == null) return;

            Data = new List<StockParam>(_allData);
            stockSet.ItemsSource = Data;
            tbRecordCount.Text = string.Format("共 {0:N0} 条记录", Data.Count);
            tbStatus.Text = "已加载";
            SyncCalendarRange();
        }

        // 图表浮窗

        private void stockChart_MouseMove(object sender, MouseEventArgs e)
        {
            if (Data == null || Data.Count == 0) return;

            double chartH = stockChart.ActualHeight;
            double chartW = stockChart.ActualWidth;
            if (double.IsNaN(chartH) || double.IsNaN(chartW) || chartH <= 0 || chartW <= 0)
                return;

            var pos = e.GetPosition(stockChart);

            if (pos.X < 0 || pos.X > chartW || pos.Y < 0 || pos.Y > chartH * 0.88)
            {
                HideTooltip();
                return;
            }

            int index = GetDataIndexAtCoordinate(pos.X, pos.Y, chartW, chartH);
            if (index < 0 || index >= Data.Count) return;

            ShowTooltip(Data[index], index, pos);
        }

        // 通过视觉树定位绘图面板，将屏幕X坐标映射为数据索引
        private int GetDataIndexAtCoordinate(double mouseX, double mouseY,
            double chartW, double chartH)
        {
            // 定位绘图面板：排除左侧数值轴标签
            double plotLeft = 0, plotWidth = chartW;
            if (!GetCandlestickPlotBounds(out plotLeft, out plotWidth, chartW, chartH))
            {
                plotLeft = 55;
                plotWidth = chartW - plotLeft - 8;
                if (plotWidth <= 0) plotWidth = chartW;
            }

            double relX = mouseX - plotLeft;
            double relW = plotWidth;

            if (relX < 0) relX = 0;
            if (relX > relW) relX = relW;

            // 方案1: DateTimeAxis 坐标→日期
            if (stockChart.Charts.Count > 0)
            {
                var chart = stockChart.Charts[0];
                DateTime? dateAtCursor = TryGetDateFromAxis(chart, relX);
                if (dateAtCursor.HasValue)
                {
                    return FindNearestByDate(dateAtCursor.Value);
                }
            }

            // 方案2: 可视日期范围线性插值
            if (stockChart.Charts.Count > 0)
            {
                var chart = stockChart.Charts[0];
                if (TryGetVisibleDateRange(chart, out DateTime visMin, out DateTime visMax))
                {
                    long ticksSpan = visMax.Ticks - visMin.Ticks;
                    if (ticksSpan > 0)
                    {
                        double ratio = relX / relW;
                        DateTime estimatedDate = visMin.AddTicks((long)(ticksSpan * ratio));
                        return FindNearestByDate(estimatedDate);
                    }
                }
            }

            // 方案3: 线性推算兜底
            int idx = (int)(relX / relW * Data.Count);
            return Math.Max(0, Math.Min(Data.Count - 1, idx));
        }

        private bool GetCandlestickPlotBounds(out double plotLeft, out double plotWidth,
            double chartW, double chartH)
        {
            plotLeft = 0;
            plotWidth = chartW;

            try
            {
                if (stockChart.Charts.Count == 0) return false;

                var chartElement = stockChart.Charts[0] as FrameworkElement;
                if (chartElement == null) return false;

                double chartElemW = chartElement.ActualWidth;
                if (double.IsNaN(chartElemW) || chartElemW <= 0)
                    chartElemW = chartW;

                FrameworkElement plotPanel = FindDeepestPlotPanel(chartElement, chartElemW);
                if (plotPanel == null) return false;

                Point panelOrigin = plotPanel.TransformToAncestor(stockChart).Transform(new Point(0, 0));
                plotLeft = panelOrigin.X;
                plotWidth = plotPanel.ActualWidth;

                if (plotWidth < 20 || plotLeft < 10 || plotLeft + plotWidth > chartW + 10)
                    return false;

                return true;
            }
            catch
            {
                return false;
            }
        }

        // 递归查找视觉树中 K 线绘图面板
        private static FrameworkElement FindDeepestPlotPanel(FrameworkElement parent, double parentWidth)
        {
            int count = VisualTreeHelper.GetChildrenCount(parent);
            if (count == 0) return null;

            FrameworkElement bestChild = null;
            double bestScore = 0;

            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(parent, i) as FrameworkElement;
                if (child == null) continue;
                if (child.ActualWidth <= 0 || child.ActualHeight <= 0) continue;

                Point childOrigin = child.TransformToAncestor(parent).Transform(new Point(0, 0));
                double childX = childOrigin.X;
                double childW = child.ActualWidth;
                double widthRatio = childW / parentWidth;

                if (childX >= 25 && widthRatio > 0.5 && childW > 100)
                {
                    double score = widthRatio * 100 + (1.0 - childX / parentWidth) * 50;
                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestChild = child;
                    }
                }

                FrameworkElement deeper = FindDeepestPlotPanel(child, childW);
                if (deeper != null)
                {
                    Point deeperOrigin = deeper.TransformToAncestor(parent).Transform(new Point(0, 0));
                    double deeperX = deeperOrigin.X;
                    double deeperRatio = deeper.ActualWidth / parentWidth;
                    if (deeperX >= 25 && deeperRatio > 0.5 && deeper.ActualWidth > 100)
                    {
                        double deeperScore = deeperRatio * 100 + (1.0 - deeperX / parentWidth) * 50;
                        if (deeperScore > bestScore)
                        {
                            bestScore = deeperScore;
                            bestChild = deeper;
                        }
                    }
                }
            }

            return bestChild;
        }

        // 反射调用 DateTimeAxis 的 CoordinateToValue / CoordinateToDate / GetDate
        private static DateTime? TryGetDateFromAxis(object chart, double mouseX)
        {
            try
            {
                PropertyInfo axisProp = chart.GetType().GetProperty("DateTimeAxis",
                    BindingFlags.Public | BindingFlags.Instance);
                if (axisProp == null) return null;

                object axis = axisProp.GetValue(chart, null);
                if (axis == null) return null;

                Type axisType = axis.GetType();

                MethodInfo method = axisType.GetMethod("CoordinateToValue",
                    BindingFlags.Public | BindingFlags.Instance, null,
                    new[] { typeof(double) }, null);
                if (method != null)
                {
                    object result = method.Invoke(axis, new object[] { mouseX });
                    if (result is DateTime dt) return dt;
                    if (result is double d && d > 0) return DateTime.FromOADate(d);
                }

                method = axisType.GetMethod("CoordinateToDate",
                    BindingFlags.Public | BindingFlags.Instance, null,
                    new[] { typeof(double) }, null);
                if (method != null)
                {
                    object result = method.Invoke(axis, new object[] { mouseX });
                    if (result is DateTime dt) return dt;
                }

                method = axisType.GetMethod("GetDate",
                    BindingFlags.Public | BindingFlags.Instance, null,
                    new[] { typeof(double) }, null);
                if (method != null)
                {
                    object result = method.Invoke(axis, new object[] { mouseX });
                    if (result is DateTime dt) return dt;
                }
            }
            catch { }

            return null;
        }

        // 反射获取图表可视日期范围
        private static bool TryGetVisibleDateRange(object chart, out DateTime min, out DateTime max)
        {
            min = default; max = default;
            try
            {
                Type chartType = chart.GetType();

                PropertyInfo pMin = chartType.GetProperty("VisibleMin",
                    BindingFlags.Public | BindingFlags.Instance);
                PropertyInfo pMax = chartType.GetProperty("VisibleMax",
                    BindingFlags.Public | BindingFlags.Instance);
                if (pMin != null && pMax != null)
                {
                    object vMin = pMin.GetValue(chart, null);
                    object vMax = pMax.GetValue(chart, null);
                    if (vMin is DateTime dMin && vMax is DateTime dMax)
                    { min = dMin; max = dMax; return true; }
                }

                pMin = chartType.GetProperty("MinDate",
                    BindingFlags.Public | BindingFlags.Instance);
                pMax = chartType.GetProperty("MaxDate",
                    BindingFlags.Public | BindingFlags.Instance);
                if (pMin != null && pMax != null)
                {
                    object vMin = pMin.GetValue(chart, null);
                    object vMax = pMax.GetValue(chart, null);
                    if (vMin is DateTime dMin && vMax is DateTime dMax)
                    { min = dMin; max = dMax; return true; }
                }

                PropertyInfo axisProp = chartType.GetProperty("DateTimeAxis",
                    BindingFlags.Public | BindingFlags.Instance);
                if (axisProp != null)
                {
                    object axis = axisProp.GetValue(chart, null);
                    if (axis != null)
                    {
                        Type axisType = axis.GetType();
                        PropertyInfo aMin = axisType.GetProperty("Minimum",
                            BindingFlags.Public | BindingFlags.Instance);
                        PropertyInfo aMax = axisType.GetProperty("Maximum",
                            BindingFlags.Public | BindingFlags.Instance);
                        if (aMin != null && aMax != null)
                        {
                            object vMin = aMin.GetValue(axis, null);
                            object vMax = aMax.GetValue(axis, null);
                            if (vMin is DateTime dMin && vMax is DateTime dMax)
                            { min = dMin; max = dMax; return true; }
                        }
                    }
                }
            }
            catch { }

            return false;
        }

        // 二分查找最接近目标日期的数据索引
        private int FindNearestByDate(DateTime target)
        {
            int lo = 0, hi = Data.Count - 1;
            while (lo < hi)
            {
                int mid = (lo + hi) / 2;
                if (Data[mid].date < target)
                    lo = mid + 1;
                else
                    hi = mid;
            }
            if (lo > 0 && Math.Abs((Data[lo - 1].date - target).Ticks) <
                         Math.Abs((Data[lo].date - target).Ticks))
                return lo - 1;
            return lo;
        }

        private void stockChart_MouseLeave(object sender, MouseEventArgs e)
        {
            Dispatcher.BeginInvoke(new Action(() =>
            {
                if (!_mouseOverTooltip)
                    HideTooltip();
            }), System.Windows.Threading.DispatcherPriority.Background);
        }

        private void tooltipBorder_MouseEnter(object sender, MouseEventArgs e)
        {
            _mouseOverTooltip = true;
        }

        private void tooltipBorder_MouseLeave(object sender, MouseEventArgs e)
        {
            _mouseOverTooltip = false;
            HideTooltip();
        }

        private void ShowTooltip(StockParam p, int index, Point mousePos)
        {
            if (chartTooltip.PlacementTarget == null)
                chartTooltip.PlacementTarget = stockChart;

            double prevClose = (index > 0) ? Data[index - 1].close : p.open;

            tooltipDate.Text = p.date.ToString("yyyy-MM-dd");
            tooltipWeekday.Text = GetChineseWeekday(p.date.DayOfWeek);
            tooltipOpen.Text = FormatPrice(p.open);

            ColorHighLow(p.high, prevClose, tooltipHigh, tooltipHighChg);
            ColorHighLow(p.low,  prevClose, tooltipLow,  tooltipLowChg);

            double chg = p.close - prevClose;
            double chgPct = (prevClose != 0) ? (chg / prevClose) * 100.0 : 0;
            bool isUp = chg >= 0;

            tooltipClose.Text = FormatPrice(p.close);
            tooltipClose.Foreground = GetBrush(isUp ? "#DC3545" : "#28A745");

            string arrow = isUp ? "▲" : "▼";
            tooltipChgPct.Text = string.Format("{0}{1:F2}%", arrow, Math.Abs(chgPct));
            tooltipChgPct.Foreground = GetBrush(isUp ? "#DC3545" : "#28A745");

            tooltipVolume.Text = FormatVolume(p.volume);

            const double popupW = 200;
            const double popupH = 140;
            double chartW = stockChart.ActualWidth;

            double offsetX = mousePos.X + 16;
            double offsetY = mousePos.Y - popupH - 8;

            if (offsetX + popupW > chartW)
                offsetX = mousePos.X - popupW - 8;
            if (offsetY < 0)
                offsetY = mousePos.Y + 16;

            if (offsetX < 0)  offsetX = 4;
            if (offsetY < 0)  offsetY = 4;

            chartTooltip.HorizontalOffset = offsetX;
            chartTooltip.VerticalOffset   = offsetY;

            if (!_tooltipActive)
            {
                chartTooltip.IsOpen = true;
                _tooltipActive = true;
            }
        }

        private void HideTooltip()
        {
            if (_tooltipActive)
            {
                chartTooltip.IsOpen = false;
                _tooltipActive = false;
            }
        }

        private static void ColorHighLow(double value, double prevClose,
            System.Windows.Controls.TextBlock valueBlock,
            System.Windows.Controls.TextBlock chgBlock)
        {
            double chg = value - prevClose;
            double chgPct = (prevClose != 0) ? (chg / prevClose) * 100.0 : 0;
            bool up = chg >= 0;

            valueBlock.Text = FormatPrice(value);
            valueBlock.Foreground = up
                ? new SolidColorBrush(Color.FromRgb(0xDC, 0x35, 0x45))
                : new SolidColorBrush(Color.FromRgb(0x28, 0xA7, 0x45));

            string arrow = up ? "▲" : "▼";
            chgBlock.Text = string.Format("{0}{1:F2}%", arrow, Math.Abs(chgPct));
            chgBlock.Foreground = valueBlock.Foreground;
        }

        // 格式化工具

        private static string FormatPrice(double p)  => p.ToString("F2", CultureInfo.InvariantCulture);
        private static string FormatVolume(double v) => v.ToString("N0", CultureInfo.InvariantCulture);

        private static SolidColorBrush GetBrush(string hex)
        {
            return (SolidColorBrush)new BrushConverter().ConvertFrom(hex);
        }

        private static string GetChineseWeekday(DayOfWeek day)
        {
            switch (day)
            {
                case DayOfWeek.Monday:    return "周一";
                case DayOfWeek.Tuesday:   return "周二";
                case DayOfWeek.Wednesday: return "周三";
                case DayOfWeek.Thursday:  return "周四";
                case DayOfWeek.Friday:    return "周五";
                case DayOfWeek.Saturday:  return "周六";
                case DayOfWeek.Sunday:    return "周日";
                default:                  return "";
            }
        }
    }
}
