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
            HookDetail hook;
            int stat = ClientRequest(argv[1], hook);
            Message("status = %d, message:\n%s", stat, hook.msg.c_str());
        }
    }

    cout << "Hello HttpAgent." << endl;
    return 0;
}
