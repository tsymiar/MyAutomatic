#include <IMclient.h>

void runtime1(void* lp) {
    static char rcv_buf[256];
    static char srv_net[32];
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    st_client* client = (st_client*)lp;
    int length = 0;
    while (1) {
        if (client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int rcvlen = recv(client->sock, rcv_buf, 256, 0);
        EnterCriticalSection(&wrcon);
        for (int c = 0; c < rcvlen; c++)
            printf("%c", (unsigned char)rcv_buf[c]);
        printf("\n");
        if (rcv_buf[1] == 0x6 && rcv_buf[2] >= 48 && rcv_buf[2] < 102) {
            MSG_trans trans{ 0x0,0x6,"0" };
            int ip = atoi(rcv_buf + 32);
            unsigned char *val = (unsigned char *)&ip;
            printf("User:\t%s\nIP:\t%u.%u.%u.%u\nPORT:\t%s\n",
                rcv_buf + 8, val[3], val[2], val[1], val[0], rcv_buf + 54);
            int parse = p2pMessage(rcv_buf + 8, atoi(rcv_buf + 8), atoi(rcv_buf + 54), (char*)&trans);
        }
        if (rcv_buf[1] == 0x8) {
            int rcv_len = atoi(rcv_buf + 22);
            FILE * file = fopen("recv.file", "ab+");
            if (length == rcv_len)
                continue;
            length = rcv_len;
            char data[224];
            memcpy(data, rcv_buf + 32, 224);
            fwrite(data, sizeof(unsigned char), 224, file);
            fclose(file);
        }
        LeaveCriticalSection(&wrcon);
        if (rcvlen <= 0) {
            sprintf(srv_net, "Connection lost!\n%s:%d", inet_ntoa(client->srvaddr.sin_addr), client->srvaddr.sin_port);
            char title[32];
            sprintf(title, "socket: %s", itoa((int)client->sock, rcv_buf, 10));
            MessageBox(NULL, srv_net, title, MB_OK);
            client->flag = -1;
            closesocket(client->sock);
            exit(0);
            break;
        };
    };
};

int main()
{
    StartChat(InitChat(), runtime1);
    while (1) {
        if (GetStatus() < 0)
            break;
        int comm = 0;
        printf("Input commond [1-13]: ");
        if (0 >= scanf("%d", &comm))
            break;
        MSG_trans msg;
        memset(&msg, 0, sizeof(MSG_trans));
        msg.uiCmdMsg = comm;
        memcpy(msg.usr, "AAA", 4);
        if (comm == 6)
            memcpy(msg.peer, "iv9527", 7);
        else
            memcpy(msg.psw, "AAA", 4);
        SetChatMsg(&msg);
        Sleep(100);
    }
    return 0;
}
