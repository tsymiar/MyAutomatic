#include "HttpSample.h"

#include "HttpEvent.h"
#include "Utils.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc <= 1) {
        cout << "Usage:\n " << (argv == nullptr ? "httpsample" : argv[0]) << " [port | url(http://...)]\nactually:" << endl;
        int i = 0;
        while (i < argc) {
            cout << " " << argv[i];
            i++;
        }
        cout << endl;
        return -1;
    } else {
        if (isNum(argv[1])) {
            short port = atoi(argv[1]);
            StartServer(port);
        } else {
            HookDetail message;
            int stat = RequestClient(argv[1], message);
            Message("status = %d, message:\n[%s]", stat, message.msg.c_str());
        }
    }
    cout << "Goodby HttpSample." << endl;
    return 0;
}
