#include "IMclient.h"

int g_printedInput = 0;
const char youSaid[11] = "Msg Sent: ";

void
#ifndef _WIN32
*
#endif
parseRcvMsg(void* lprcv) {
    static char rcv_buf[256];
    static char srv_net[32];
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    st_client* client = (st_client*)lprcv;
    int fw_len = 0;
    int count = 0;
    while (1) {
        if (client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int rcvlen = recv(client->sock, rcv_buf, 256, 0);
        EnterCriticalSection(&wrcon);
        if (rcvlen == 2 && memcmp(rcv_buf, "\0\0", 2) == 0)
            continue;
        st_trans *mesg = (st_trans*)rcv_buf;
#ifdef _DEBUG
        if (mesg->uiCmdMsg != CHATWITH) {
            for (int c = 0; c < rcvlen; c++)
                fprintf(stdout, "%c", (unsigned char)rcv_buf[c]);
            fprintf(stdout, "\n");
            SetRecvState(RCV_TCP);
        } else {
            SetRecvState(RCV_ERR);
            if (g_printedInput >= 0) {
                fprintf(stdout, "\rRecieving...");
            } else {
                fprintf(stdout, "\r%*c\rRecieving...\n%s", 64, ' ', youSaid);
            }
        }
#endif
        if (mesg->uiCmdMsg == PEER2P) {
            int ip = atoi((const char*)mesg->peerIP);
            unsigned char *val = (unsigned char *)&ip;
            int port = atoi((const char*)mesg->peer_port);
            fprintf(stdout, "User:\t%s\nIP:\t%u.%u.%u.%u\nPORT:\t%d\n",
                mesg->username, val[3], val[2], val[1], val[0], port);
            if (port <= 0) {
                fprintf(stdout, "Error number of user port, stop send p2p message!\n");
            } else {
                struct MoreMesg p2pmsg;
                memset(&p2pmsg, 0, sizeof(MoreMesg));
                p2pmsg.cmd[0] = mesg->uiCmdMsg;
                memcpy(&p2pmsg.mesg, "hello", 16);
                int parse = p2pMessage(mesg->username, ip, port, (char*)&p2pmsg);
                if (parse == 0) {
                    SetRecvState(RCV_P2P);
                }
            }
        }
        if (mesg->uiCmdMsg == CHATWITH) {
            if (atoi((const char*)mesg->recv_mesg.status) == 200) {
                SetRecvState(RCV_NDT);
                fprintf(stdout, "\r%*c\rRecieved: %s\n", 64, ' ', mesg->recv_mesg);
                if (g_printedInput > 0) {
                    fprintf(stdout, youSaid);
                }
            } else if (rcvlen < 0) {
                fprintf(stdout, "Status error.\n");
                closesocket(client->sock);
                exit(0);
            }
        }
        if (mesg->uiCmdMsg == GETIMAGE) {
            int ndt_len = atoi(rcv_buf + 22);
            if (count = 0) {
                fclose(fopen(filename, "w"));
                count = 1;
            }
            FILE * file = fopen(filename, "ab+");
            if (fw_len == ndt_len) {
                fclose(file);
                continue;
            }
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
        if (rcvlen <= 0 || (mesg->value[0] == 'e' && mesg->value[1] == '8')) {
            sprintf(srv_net, "Connection lost!\n%s:%d", inet_ntoa(client->srvaddr.sin_addr), client->srvaddr.sin_port);
            char title[32];
            sprintf(title, "socket: %s", _itoa((int)client->sock, (char*)mesg, 10));
            MessageBox(NULL, srv_net, title, MB_OK);
            SetChatActive(rcvlen);
            closesocket(client->sock);
            exit(0);
            break;
        };
    };
};

st_trans* SetChatMesg(st_trans& trans) {
    switch (trans.uiCmdMsg)
    {
    case REGISTER:
        break;
    case LOGIN:
    {
        static char title[64];
        if (trans.username != NULL) {
            sprintf(title, "Welcome %s", (char*)trans.username);
            SetConsoleTitle(title);
        }
        break;
    }
    case SETPSW:
    {
        gets_s((char*)trans.password, 24);
        break;
    }
    case PEER2P:
    {
        break;
    }
    case CHATWITH:
    {
        memcpy(trans.type, "NDT", 4);
        trans.more_mesg.cmd[0] = CHATWITH;
#ifdef NDT_ONLY
        if (g_printedInput > -1) {
            fprintf(stdout, youSaid);
            g_printedInput = 1;
        }
#else
        fprintf(stdout, "Input chat message, limit on 16 characters.\n");
        g_printedInput = 1;
#endif
        memset(&trans.more_mesg.mesg, 0, 16);
        if (scanf_s("%s", &trans.more_mesg.mesg, 16) <= 0) {
            fprintf(stdout, "Error: characters limit 16!\n");
            g_printedInput = trans.uiCmdMsg = -1;
            return &trans;
        }
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
        scanf_s("%s %s", trans.username, (unsigned)_countof(trans.username), (trans.group_join), (unsigned)_countof(trans.group_join));
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
        if (trans.value == 0x0)
        {
            MessageBox(NULL, "Logging failure.", "message", MB_OK);
            return &trans;
        }
    }
    }
    return &trans;
}

int main()
{
    if (StartChat(InitChat(), parseRcvMsg) < 0)
        return -1;
#ifdef NDT_ONLY
    int comm = 1;
#endif
    while (1) {
        if (IsChatActive() < 0) {
            return CloseChat();
        }
        st_trans msg;
        memset(&msg, 0, sizeof(st_trans));
        int recieved = GetRecvState();
        g_printedInput = 0;
        memcpy(msg.username, "AAAAA", 6);
        memcpy(msg.password, "AAAAA", 6);
        memcpy(msg.peer_name, "iv9527", 7);
#ifdef NDT_ONLY
        if (comm > 1) {
            comm = CHATWITH;
        }
        msg.more_mesg.cmd[0] = msg.uiCmdMsg = comm;
        while (IsChatActive() == 0) { ; }
        comm++;
#else
        int comm = 0;
        if (recieved == RCV_TCP || recieved == RCV_SCC) {
            fprintf(stdout, "Input commond [1-13]: ");
            if (scanf("%2d", &comm) <= 0) {
                fprintf(stdout, "Input object format error.\n");
                break;
            }
            if (comm > 13 || comm < 1) {
                fprintf(stdout, "Input value error: beyound [1,13].\n");
                break;
            }
        }
        msg.uiCmdMsg = comm;
#endif // NDT_ONLY
        if (recieved == RCV_NDT) {
            msg.more_mesg.cmd[0] = msg.uiCmdMsg = CHATWITH;
        }
        if (recieved > RCV_ERR) {
            int ret = SendChatMesg(SetChatMesg(msg));
            if (ret < 0) {
                fprintf(stdout, "Error while setting chat message.\n");
                return -1;
            }
        }
        Sleep(100);
    }
    return 0;
}
