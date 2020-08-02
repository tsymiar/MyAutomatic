using System;
using System.CodeDom;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows;

#pragma warning disable 1591

namespace WPFKline
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        String stockName;

        public List<StockParam> Data { get; set; }

        public MainWindow()
        {
            InitializeComponent();
        }

        private List<StockParam> LoadStockInfo(string fileName)
        {
            if (File.Exists(fileName))
            {
                Stream resourceStream = null;
                try
                {
                    resourceStream = new FileStream(fileName, FileMode.Open);
                    using (StreamReader reader = new StreamReader(resourceStream, Encoding.GetEncoding("GB2312")))
                    {
                        //读每一行
                        var strings = reader.ReadToEnd().Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);
                        //获取股票名称
                        stockName = strings[0].Replace("\r", "");

                        var res = new List<StockParam>(strings.Length - 2);

                        //第一行是股票名称, 第二行是类型名称, 第3行才是股票数据
                        for (int i = 2; i < strings.Length; i++)
                        {
                            string line = strings[i];
                            string[] subLines = line.Split('\t');

                            DateTime date = DateTime.Parse(subLines[0]);
                            Double open = Double.Parse(subLines[1]);
                            Double high = Double.Parse(subLines[2]);
                            Double low = Double.Parse(subLines[3]);
                            Double close = Double.Parse(subLines[4]);
                            Double volumn = Double.Parse(subLines[5]);

                            res.Add(
                                new StockParam
                                {
                                    date = date,
                                    open = open,
                                    high = high,
                                    low = low,
                                    close = close,
                                    volume = volumn
                                });
                        }
                        return res;
                    }
                }
                finally
                {
                    if (resourceStream != null)
                        resourceStream.Dispose();
                }
            }
            else
            {
                DirectoryInfo fileInfo = new DirectoryInfo(fileName);
                System.Windows.MessageBox.Show("No such [" + fileInfo.Name + "] file!", "WARN");
                return null;
            }
        }
        private void LoadFile()
        {
            string rootpath = System.Windows.Forms.Application.StartupPath;// AppDomain.CurrentDomain.BaseDirectory;
            if (!String.IsNullOrEmpty(rootpath))
            {
                txtFilePath.Text = rootpath +
#if DEBUG
                    @"\data\SH600747.DAT";
#else
                @"\..\..\..\..\MfcUtil\data\SH600747.DAT";
#endif
                LoadData(txtFilePath.Text);
                stockSet.ItemsSource = Data;
            }
        }
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.DataContext = this;
            LoadFile();
        }

        private void LoadData(string path)
        {
            Data = LoadStockInfo(path);
            //stockChart.Charts[0].Collapse();
            stockChart.Charts[0].Graphs[0].Title = stockName;
            stockChart.Charts[1].Graphs[0].Title = stockName;
        }

        private void btnOpenFile_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.OpenFileDialog ofd = new Microsoft.Win32.OpenFileDialog();
            ofd.Filter = "coord(*.DAT)|*.DAT";
            ofd.RestoreDirectory = true;
            ofd.ShowDialog();
            if (!String.IsNullOrEmpty(ofd.FileName))
            {
                txtFilePath.Text = ofd.FileName;
                LoadData(ofd.FileName);
                stockSet.ItemsSource = Data;
            }
        }

    }
}
