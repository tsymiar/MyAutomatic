#include "WebMain.h"

#include "WebevApi.h"
#include "Utils.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc <= 1) {
        cout << "Usage:\n " << (argv == nullptr ? "./webevent_server" : argv[0]) << " [ port || url(http://...) ]\n";
        cout << " frontend: env WEBEV_FRONTEND=\"branch[:dir]\" (default \"gh-pages:./webev-frontend\", 0=disable)\n";
        cout << "           env WEBEV_REPO=/path/to/repo to point at the git repository (auto-detected otherwise)\n";
        cout << "actually:" << endl;
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
    cout << "Goodby webevent_server." << endl;
    return 0;
}
