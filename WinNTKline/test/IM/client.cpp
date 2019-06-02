#include "IMclient.h"

void
#ifndef _WIN32
*
#endif
parseRcvMsg(void* lp) {
    static char rcv_buf[256];
    static char srv_net[32];
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    st_client* client = (st_client*)lp;
    int fw_len = 0;
    int count = 0;
    while (1) {
        if (client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int rcvlen = recv(client->sock, rcv_buf, 256, 0);
        EnterCriticalSection(&wrcon);
#ifdef _DEBUG
        for (int c = 0; c < rcvlen; c++)
            fprintf(stdout, "%c", (unsigned char)rcv_buf[c]);
        fprintf(stdout, "\n");
#endif
        st_trans *mesg = (st_trans*)rcv_buf;
        if (mesg->uiCmdMsg == 0x6 && memcmp(mesg->retval, "ff", 2) == 0) {
            int ip = atoi((const char*)mesg->peerIP);
            unsigned char *val = (unsigned char *)&ip;
            int port = atoi((const char*)mesg->peer_port);
            fprintf(stdout, "User:\t%s\nIP:\t%u.%u.%u.%u\nPORT:\t%d\n",
                mesg->usr, val[3], val[2], val[1], val[0], port);
            if (port <= 0) {
                fprintf(stdout, "Error number of user port, stop send p2p message!\n");
            } else {
                st_trans trans{ 0x0,0x6, "0" };
                int parse = p2pMessage(mesg->usr, ip, port, (char*)&trans);
            }
        }
        if (mesg->uiCmdMsg == 0x8) {
            int ndt_len = atoi(rcv_buf + 22);
            if (count = 0)
                fclose(fopen(filename, "w"));
            FILE * file = fopen(filename, "ab+");
            if (fw_len == ndt_len)
                continue;
            fw_len = ndt_len;
            char data[224];
            memcpy(data, rcv_buf + 32, 224);
            if (fw_len - (fw_len / 224) * 224 > 0)
                fwrite(data, sizeof(unsigned char), fw_len % 224, file);
            else
                fwrite(data, sizeof(unsigned char), 224, file);
            fclose(file);
        }
        LeaveCriticalSection(&wrcon);
        if (rcvlen <= 0 || (mesg->retval[0] == 'e' && mesg->retval[1] == '8')) {
            sprintf(srv_net, "Connection lost!\n%s:%d", inet_ntoa(client->srvaddr.sin_addr), client->srvaddr.sin_port);
            char title[32];
            sprintf(title, "socket: %s", _itoa((int)client->sock, (char*)mesg, 10));
            MessageBox(NULL, srv_net, title, MB_OK);
            client->flag = -1;
            closesocket(client->sock);
            exit(0);
            break;
        };
        count++;
    };
};

st_trans* SetChatMsg(st_trans& trans) {
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
    case PEER2P:
    {
        break;
    }
    case CHATWITH: 
    {
        memcpy(trans.type, "NDT", 4);
        fprintf(stdout, "Input chat message, limit on 16 characters.\n");
        memset(trans.peer_mesg.head, 0, 16);
        trans.peer_mesg.cmd[0] = CHATWITH;
        scanf_s("%16s", trans.peer_mesg.head, 16);
        break;
    }
    case HOSTGROUP:
    {
        fprintf(stdout, "Input host group name AND group mark, divide with BLANK(' ').\n");
        scanf_s("%s %s", trans.group_host, (unsigned)_countof(trans.group_host), (trans.group_mark), (unsigned)_countof(trans.group_mark));
        break;
    }
    case JOINGROUP:
    {
        fprintf(stdout, "Input user name AND group name want to join, divide with BLANK(' ').\n");
        scanf_s("%s %s", trans.usr, (unsigned)_countof(trans.usr), (trans.group_join), (unsigned)_countof(trans.group_join));
        break;
    }
    case VIEWGROUP:
    {
        fprintf(stdout, "Input group name limit on 24 characters.\n");
        scanf_s("%s", trans.group_name, 24);
        break;
    }
    default:
    {
        if (trans.retval == 0x0)
        {
            MessageBox(NULL, "Logging failure.", "default", MB_OK);
            return &trans;
        }
    }
    }
    return &trans;
}

int main()
{
    StartChat(InitChat(), parseRcvMsg);
    while (1) {
        if (GetStatus() < 0)
            break;
        int comm = 0;
        fprintf(stdout, "Input commond [1-13]: ");
        if (scanf("%d", &comm) <= 0)
            break;
        st_trans msg;
        memset(&msg, 0, sizeof(st_trans));
        msg.uiCmdMsg = comm;
        memcpy(msg.usr, "AAA", 4);
        memcpy(msg.psw, "AAA", 4);
        memcpy(msg.peer_name, "iv9527", 7);
        msg = *SetChatMsg(msg);
        SendChatMsg(&msg);
        Sleep(100);
    }
    return 0;
}
