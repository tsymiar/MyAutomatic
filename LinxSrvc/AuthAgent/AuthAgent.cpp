#include "AuthAgent.h"

#include "HttpEvent.h"
#include "Logging.h"
#include "utils.h"

using namespace std;

int main(int argc, char** argv)
{
    int i = 0;
    Message("usage: ");
    while (i < argc) {
        Message("%s", argv[i]);
        i++;
    }

    short port = 8080;
    if (argc > 1 && isNum(argv[1])) {
        port = atoi(argv[1]);
    }
    StartServer(port);

    cout << "Hello AuthAgent." << endl;
    return 0;
}
