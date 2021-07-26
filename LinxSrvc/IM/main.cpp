#include "KaiSocket.h"

#ifdef __linux
#include <unistd.h>
#endif

int reciever(KaiSocket* kaisock) {
    char* rcv_txt = new char[64];
    memset(rcv_txt, 0, 64);
    int rcvlen = kaisock->recv(rcv_txt, 64);
    if (rcvlen > 0) {
        for (int c = 0; c < rcvlen; c++) {
            if (c > 0 && c % 32 == 0)
                fprintf(stdout, "\n");
            fprintf(stdout, "%02x ", static_cast<unsigned char>(rcv_txt[c]));
        }
        fprintf(stdout, "\n");
    }
    delete[]rcv_txt;
    return 0;
}

int sender(KaiSocket* kaisock) {
    char* rcv_txt = new char[64];
    memset(rcv_txt, 0, 64);
    scanf("%s\n", rcv_txt);
    kaisock->send(rcv_txt, 64);
    fprintf(stdout, "----------------\n");
    delete[]rcv_txt;
    return 0;
}

int main(int argc, char* argv[])
{
    bool client = false;
    if (argc > 1) {
        client = (argv[1] == "-C" ? true : false);
    }
#ifdef __linux
    if (fork() == 0) {
#endif
        KaiSocket* kai = nullptr;
        if (client) {
            kai = new KaiSocket("192.168.1.1", 9999);
        } else {
            kai = new KaiSocket(9999);
        }
        kai->registCallback(reciever);
        kai->appendCallback(sender);
        if (client)
            kai->connect();
        else
            kai->start();
        delete kai;
#ifdef __linux
    }
#endif
}
