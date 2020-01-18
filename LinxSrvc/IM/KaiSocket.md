### KaiSocket Usage

##### Server:
First of all, create a new instance with sentence `KaSocket* server = new KaSocket(PORT);`. Then call `setCallback` to register a callback function in format `void (*)(void*)`, the callback can used to `recv` or `send` data in each other's loop block without block, because of running on threads. Also, add other callbacks in the same format by calling `addCallback`, therefore, just need to be attention to is how dealing the data rather than the pesky socket transmission. At last, call `start()` to start a new 'KaiSocket' server process.
* e.g:
```c
#include "KaSocket.h"
#include <string.h>
#include <unistd.h>
void parser0(void* lprcv) {
    KaSocket* kasock = (KaSocket*)lprcv;
    char rcv_txt[64];
    while (kasock->running()) {
        memset(rcv_txt, 0, 64);
        int rcvlen = kasock->recv(rcv_txt, 64);
        if (rcvlen > 0) {
            for (int c = 0; c < rcvlen; c++) {
                if (c > 0 && c % 32 == 0)
                    fprintf(stdout, "\n");
                fprintf(stdout, "%02x ", static_cast<unsigned char>(rcv_txt[c]));
            }
            fprintf(stdout, "\n");
        }
    }
}
void parser1(void* lprcv) {
    KaSocket* kasock = (KaSocket*)lprcv;
    char rcv_txt[64];
    while (kasock->running()) {
        memset(rcv_txt, 0, 64);
        scanf("%s", rcv_txt);
        kasock->send(rcv_txt, 64);
        fprintf(stdout, "----------------\n");
    }
}
int main() 
{
    if (fork() == 0) {
        KaSocket* server = new KaSocket(9999);
        server->setCallback(parser0);
        server->addCallback(parser1);
        server->start();
        delete server;
    }
}
```
##### Client:
Writing a client is more simple. Just use `parser0` and `parser1` above as callback functions, and then only need to do is using `connect()` to connect a server whoes IP and PORT sets when calling `KaSocket* client = new KaSocket(IP, PORT);` to create a new client instance.
* e.g:
```c
int main()
{
    KaSocket* client = new KaSocket("192.168.1.1", 9999);
    client->setCallback(parser0);
    client->addCallback(parser1);
    client->connect();
    while (1) { client->wait(100); }
}
```
