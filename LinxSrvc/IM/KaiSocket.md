### KaiSocket Usage

##### Server:
First of all, create a new instance with sentence `KaiSocket* server = new KaiSocket(PORT);`. Then call `setCallback` to register a callback function in format `int(*)(KaiSocket*)`, the callback can used to `recv` or `send` data in each other's loop block without block, because of running on threads. Also, add other callbacks in the same format by calling `addCallback`, therefore, just need to be attention to is how dealing the data rather than the pesky socket transmission. At last, call `start()` to start a new 'KaiSocket' server process.
* e.g:
```c
#include "KaiSocket.h"
#include <string.h>
#include <unistd.h>
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
    delete []rcv_txt;
    return 0;
}
int sender(KaiSocket* kaisock) {
    char* rcv_txt = new char[64];
    memset(rcv_txt, 0, 64);
    scanf("%s", rcv_txt);
    kaisock->send(rcv_txt, 64);
    fprintf(stdout, "----------------\n");
    delete []rcv_txt;
    return 0;
}
```
```c
int main() 
{
    if (fork() == 0) {
        KaiSocket* server = new KaiSocket(9999);
        server->registCallback(reciever);
        server->appendCallback(sender);
        server->start();
        delete server;
    }
}
```
##### Client:
Writing a client is more simple. Just use `reciever` and `sender` above as callback functions, and then only need to do is using `connect()` to connect a server whoes IP and PORT sets when calling `KaiSocket* client = new KaiSocket(IP, PORT);` to create a new client instance.
* e.g:
```c
int main()
{
    KaiSocket* client = new KaiSocket("192.168.1.1", 9999);
    client->registCallback(reciever);
    client->appendCallback(sender);
    client->connect();
    delete client;
}
```
