#include "AuthAgent.h"

#include "HttpEvent.h"
#include "Logging.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc > 1) {
        Message("usage: %s", argv[1]);
        return 1;
    }

    StartServer(8081);

    cout << "Hello CMake." << endl;
    return 0;
}
