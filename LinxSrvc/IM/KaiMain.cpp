#include "KaiSocket.h"

#include <unistd.h>
#include <iostream>

int reciever(KaiSocket* kai) {
    char* text = new char[64];
    memset(text, 0, 64);
    int rcvlen = kai->recv(text, 64);
    if (rcvlen > 0) {
        for (int c = 0; c < rcvlen; c++) {
            if (c > 0 && c % 32 == 0)
                fprintf(stdout, "\n");
            fprintf(stdout, "%02x ", static_cast<unsigned char>(text[c]));
        }
        fprintf(stdout, "\n");
    }
    delete[]text;
    return 0;
}

int sender(KaiSocket* kai) {
    char* text = new char[64];
    memset(text, 0, 64);
    char c;
    while ((std::cin >> text).get(c)) {
        kai->send(text, 64);
        fprintf(stdout, "----------------\n");
        if (c == '\n') {
            break;
        }
    }
    delete[]text;
    return 0;
}

int main(int argc, char* argv[])
{
    bool client = false;
    if (argc > 1) {
        client = (std::string(argv[1]) == "-C" ? true : false);
    }
    if (fork() == 0) {
        KaiSocket* kai = nullptr;
        if (client) {
            kai = new KaiSocket("192.168.1.1", 9999);
        } else {
            kai = new KaiSocket(9999);
        }
        kai->registerCallback(reciever);
        kai->appendCallback(sender);
        if (client)
            kai->connect();
        else
            kai->start();
        delete kai;
    }
}
