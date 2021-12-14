#include "HttpAgent.h"

#include "HttpEvent.h"
#include "Utils.h"

using namespace std;

int main(int argc, char** argv)
{
    cout << "Usage: " << argv[0] << " port[url], actually:" << endl;
    int i = 0;
    while (i < argc) {
        cout << " " << argv[i];
        i++;
    }
    cout << endl;

    short port = 8080;
    if (argc > 1) {
        if (isNum(argv[1])) {
            port = atoi(argv[1]);
            StartServer(port);
        } else {
            HookDetail message;
            int stat = RequestClient(argv[1], message);
            Message("status = %d, message:\n[%s]", stat, message.msg.c_str());
        }
    }

    cout << "Hello HttpAgent." << endl;
    return 0;
}
