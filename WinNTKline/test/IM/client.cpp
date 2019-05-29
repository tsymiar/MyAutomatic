#include <IMclient.h>

void parseRcvMsg(void* lp) {
    static char rcv_buf[256];
    static char srv_net[32];
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    st_client* client = (st_client*)lp;
    int fw_len = 0;
    int cnt = 0;
    while (1) {
        if (client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int rcvlen = recv(client->sock, rcv_buf, 256, 0);
        EnterCriticalSection(&wrcon);
#ifdef _DEBUG
        for (int c = 0; c < rcvlen; c++)
            printf("%c", (unsigned char)rcv_buf[c]);
        printf("\n");
#endif
        if (rcv_buf[1] == 0x6 && memcmp(rcv_buf + 2, "ff", 2) == 0) {
            MSG_trans trans { 0x0,0x6,"0" };
            int ip = atoi(rcv_buf + 32);
            unsigned char *val = (unsigned char *)&ip;
            printf("User:\t%s\nIP:\t%u.%u.%u.%u\nPORT:\t%s\n",
                rcv_buf + 8, val[3], val[2], val[1], val[0], rcv_buf + 54);
            int parse = p2pMessage(rcv_buf + 8, atoi(rcv_buf + 8), atoi(rcv_buf + 54), (char*)&trans);
        }
        if (rcv_buf[1] == 0x8) {
            int rcv_len = atoi(rcv_buf + 22);
            if (cnt = 0)
                fclose(fopen(filename, "w"));
            FILE * file = fopen(filename, "ab+");
            if (fw_len == rcv_len)
                continue;
            fw_len = rcv_len;
            char data[224];
            memcpy(data, rcv_buf + 32, 224);
            if (fw_len - (fw_len / 224) * 224 > 0)
                fwrite(data, sizeof(unsigned char), fw_len % 224, file);
            else
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
        cnt++;
    };
};

MSG_trans SetChatMsg(MSG_trans& trans) {
    switch (trans.uiCmdMsg)
    {
    case REGISTER:
        break;
    case LOGIN:
    {
        static char title[64];
        if (trans.usr != NULL) {
            sprintf(title, "Welcome %s", (char*)trans.usr);
            SetConsoleTitle(title);
        }
        break;
    }
    case SETPSW:
    {
        gets_s((char*)trans.psw, 24);
        break;
    }
    case HOSTGROUP:
    {
        scanf_s("%s %s", trans.hgrp, (unsigned)_countof(trans.hgrp), (trans.grpmrk), (unsigned)_countof(trans.grpmrk));
        break;
    }
    case JOINGROUP:
    {
        scanf_s("%s %s", trans.usr, (unsigned)_countof(trans.usr), (trans.jgrp), (unsigned)_countof(trans.jgrp));
        break;
    }
    case VIEWGROUP:
    {
        scanf_s("%s", trans.grpnm, 24);
        break;
    }
    default:
    {
        if (trans.rtn == 0x0)
        {
            MessageBox(NULL, "Logging failure.", "default", MB_OK);
            return trans;
        }
    }
    }
    return trans;
}

int main()
{
    StartChat(InitChat(), parseRcvMsg);
    while (1) {
        if (GetStatus() < 0)
            break;
        int comm = 0;
        printf("Input commond [1-13]: ");
        if (scanf("%d", &comm) <= 0)
            break;
        MSG_trans msg;
        memset(&msg, 0, sizeof(MSG_trans));
        msg.uiCmdMsg = comm;
        memcpy(msg.usr, "AAA", 4);
        memcpy(msg.psw, "AAA", 4);
        memcpy(msg.peer, "iv9527", 7);
        SendChatMsg(&SetChatMsg(msg));
        Sleep(100);
    }
    return 0;
}
