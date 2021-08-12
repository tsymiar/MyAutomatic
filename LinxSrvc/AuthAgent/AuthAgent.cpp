#include "AuthAgent.h"

#include "HttpEvent.h"
#include "Utils.h"

using namespace std;

int main(int argc, char** argv)
{
    int i = 0;
    cout << "Usage:" << endl;
    while (i < argc) {
        cout << " " << argv[i];
        i++;
    }
    cout << endl;

    short port = 8080;
    if (argc > 1 && isNum(argv[1])) {
        port = atoi(argv[1]);
    }
    StartServer(port);

    cout << "Hello AuthAgent." << endl;
    return 0;
}
