using namespace System::Windows;
using namespace System::Windows::Interop;
using namespace System::Runtime;
using namespace WPFKline;

ref class WPFhost
{
public:
    WPFhost() {}
    virtual ~WPFhost() {}
    static WPFKline::MainWindow ^WPFWindow;
};
