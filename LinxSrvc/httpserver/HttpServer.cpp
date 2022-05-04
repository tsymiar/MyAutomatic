#include "HttpServer.h"

#include "HttpEvent.h"
#include "Utils.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc <= 1) {
        cout << "Usage:\n " << (argv == nullptr ? "HttpServer" : argv[0]) << " [port | url]\nactually:" << endl;
    }

    int i = 0;
    while (i < argc) {
        cout << " " << argv[i];
        i++;
    }
    cout << endl;

    if (argc > 1) {
        short port = 8080;
        if (isNum(argv[1])) {
            port = atoi(argv[1]);
            StartServer(port);
        } else {
            HookDetail message;
            int stat = RequestClient(argv[1], message);
            Message("status = %d, message:\n[%s]", stat, message.msg.c_str());
        }
    }

    cout << "Goodby HttpServer." << endl;
    return 0;
}
