#include "KaiSocket.h"
#include <string.h>
// #include <unistd.h>

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

int main()
{
    //    if (fork() == 0) {
    KaiSocket* kai = nullptr;
#ifdef CLIENT
    kai = new KaiSocket("192.168.1.1", 9999);
#else
    kai = new KaiSocket(9999);
#endif
    kai->registCallback(reciever);
    kai->appendCallback(sender);
#ifdef CLIENT
    kai->connect();
#else
    kai->start();
#endif
    delete kai;
    //    }
}
