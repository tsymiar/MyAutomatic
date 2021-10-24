#include "KaiSocket.h"

#ifndef _WIN32
#include <unistd.h>
#endif
#include <iostream>

using namespace std;

int hook0(KaiSocket* kai);
int hook1(KaiSocket* kai);

int main(int argc, char* argv[]) {
    KaiRoles role = SERVER;
    if (argc > 1) {
        string argv1 = string(argv[1]);
        role = (argv1 == "-C" ? CLIENT :
            (argv1 == "-S" ? SUBSCRIBE :
                (argv1 == "-P" ? PUBLISH :
                    (argv1 == "-B" ? BROKER : SERVER))));
    }
#ifndef _WIN32
    pid_t child = fork();
    if (child == 0) {
#endif
        KaiSocket kai;
        const int PORT = 9999;
        const char* IP = "127.0.0.1";
        if (role >= CLIENT) {
            kai.Initialize(IP, PORT);
        } else {
            kai.Initialize(nullptr, PORT);
        }
        if (role == CLIENT || role == SERVER) {
            kai.registerCallback(hook0);
            kai.appendCallback(hook1);
        }
        cout << argv[0] << ": run as [" << role << "](" << KaiSocket::G_KaiRole[role] << ")" << endl;
        string topic = "topic";
        if (argc > 2) {
            topic = string(argv[2]);
        }
        string payload = "a123+/";
        switch (role) {
        case CLIENT:
            kai.connect();
            break;
        case SERVER:
            kai.start();
            break;
        case BROKER:
            kai.Broker();
            break;
        case SUBSCRIBE:
            kai.Subscriber(topic);
            break;
        case PUBLISH:
            if (argc > 3) {
                payload = string(argv[3]);
            }
            kai.Publisher(topic, payload);
        default:
            break;
        }
#ifndef _WIN32
    } else if (child > 0) {
        cout << "child process " << child << " started" << endl;
    } else {
        cout << "KaiSocket fork process failed!" << endl;
    }
#endif
}

int hook0(KaiSocket* kai) {
    uint8_t* text = new uint8_t[64];
    memset(text, 0, 64);
    int len = kai->recv(text, 64);
    if (len > 0) {
        for (int c = 0; c < len; c++) {
            if (c > 0 && c % 32 == 0)
                fprintf(stdout, "\n");
            fprintf(stdout, "%02x ", static_cast<unsigned char>(text[c]));
        }
        fprintf(stdout, "\n");
    }
    delete[]text;
    return 0;
}

int hook1(KaiSocket* kai) {
    auto* text = new uint8_t[64];
    memset(text, 0, 64);
    char c;
    (cin >> text).get(c);
    kai->send(text, 64);
    cout << "----------------" << endl;
    delete[]text;
    return 0;
}
