#include "AuthAgent.h"

#include "HttpEvent.h"
#include "Logging.h"

using namespace std;

int main(int argc, char** argv)
{
    int i = 0;
    Message("usage: ");
    while (i < argc) {
        Message("%s", argv[i]);
        i++;
    }

    StartServer(8080);

    cout << "Hello CMake." << endl;
    return 0;
}
