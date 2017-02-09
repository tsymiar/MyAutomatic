using namespace System::Windows;
using namespace System::Windows::Interop;
using namespace System::Runtime;
using namespace WpfKline;

ref class WpfHost
{
public:
	WpfHost(){}
	virtual ~WpfHost(){}
	static WpfKline::MainWindow ^WpfWindow;
};
