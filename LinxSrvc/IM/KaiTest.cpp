#include "KaiSocket.h"

#include <unistd.h>
#include <iostream>

using namespace std;

enum Rules {
    SERVER,
    BROKER,
    CLIENT,
    PUBLISH,
    SUBSCRIBE
};

int hook0(KaiSocket* kai) {
    char* text = new char[64];
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

int main(int argc, char* argv[]) {
    Rules rule = SERVER;
    if (argc > 1) {
        string argv1 = string(argv[1]);
        rule = (argv1 == "-C" ? CLIENT :
            (argv1 == "-S" ? SUBSCRIBE :
                (argv1 == "-P" ? PUBLISH :
                    (argv1 == "-B" ? BROKER : SERVER))));
    }
    pid_t child = fork();
    if (child == 0) {
        KaiSocket* kai;
        if (rule >= CLIENT) {
            kai = new(nothrow)KaiSocket("127.0.0.1", 9999);
        } else {
            kai = new(nothrow)KaiSocket(9999);
        }
        if (rule == CLIENT || rule == SERVER) {
            kai->registerCallback(hook0);
            kai->appendCallback(hook1);
        }
        char values[][10] = { "SERVER", "BROKER", "CLIENT", "PUBLISH", "SUBSCRIBE" };
        cout << argv[0] << ": run as " << rule << "[" << values[rule] << "]" << endl;
        string topic = "topic";
        if (argc > 2) {
            topic = string(argv[2]);
        }
        string payload = "a123+/";
        switch (rule) {
        case CLIENT:
            kai->connect();
            break;
        case SERVER:
            kai->start();
            break;
        case BROKER:
            kai->broker();
            break;
        case SUBSCRIBE:
            kai->Subscriber(topic);
            break;
        case PUBLISH:
            if (argc > 3) {
                payload = string(argv[3]);
            }
            kai->Publisher(topic, payload);
        default:
            break;
        }
        delete kai;
    } else if (child > 0) {
        cout << "child process " << child << " started" << endl;
    } else {
        cout << "kai-socket fork failed" << endl;
    }
}