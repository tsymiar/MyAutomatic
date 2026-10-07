#include "WebMain.h"

#include "WebevApi.h"
#include "WsServe.h"
#include "Utils.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc <= 1) {
        cout << "Usage:\n " << (argv == nullptr ? "./webevs_serve" : argv[0]) << " [ port || url(http://...) ] [ ws_port ]\n";
        cout << " frontend: env WEBEV_FRONTEND=\"branch[:dir]\" (default \"gh-pages:./webevs-frontend\", 0=disable)\n";
        cout << "           env WEBEV_REPO=/path/to/repo to point at the git repository (auto-detected otherwise)\n";
        cout << " websocket: optional 2nd arg (ws_port) or env WEBEV_WS_PORT starts a standalone libwebsockets server\n";
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
            int port = atoi(argv[1]);
            int wsPort = 0;
            if (argc > 2 && isNum(argv[2])) {
                wsPort = atoi(argv[2]);
            } else {
                const char* env = getenv("WEBEV_WS_PORT");
                if (env != nullptr && isNum(env))
                    wsPort = atoi(env);
            }
            // short caps at 32767, so validate the port as int before passing it on
            if (port <= 0 || port > 65535) {
                cerr << "invalid http port: " << port << " (expect 1~65535)" << endl;
                return -1;
            }
            if (wsPort != 0 && (wsPort <= 0 || wsPort > 65535)) {
                cerr << "invalid websocket port: " << wsPort << " (expect 1~65535)" << endl;
                return -1;
            }
            if (wsPort > 0)
                WsStartServer(wsPort);
            StartServer(port);
            WsStopServer();
        } else {
            HookDetail message;
            int stat = RequestClient(argv[1], message);
            Message("status = %d, message:\n[%s]", stat, message.msg.c_str());
        }
    }
    cout << "Goodby webevs_serve." << endl;
    return 0;
}
