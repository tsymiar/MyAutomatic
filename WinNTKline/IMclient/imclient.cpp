#include "client.h"

#define USR_TEST
volatile int g_printed = 0;
const char youSaid[11] = "Msg Sent: ";

void
#ifndef _WIN32
*
#endif
parseRcvMsg(void* lprcv)
{
    static char rcv_buf[256];
    static char srv_net[32];
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    st_client* client = (st_client*)lprcv;
    int fw_len = 0;
    int ui_val = 0;
    volatile int count = 0;
    while (1) {
        if (client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int rcvlen = recv(client->sock, rcv_buf, 256, 0);
        if (rcvlen == 2 && (rcv_buf[1] == '\0')) {
            SLEEP(1);
            continue;
        }
        EnterCriticalSection(&wrcon);
        st_trans* mesg = (st_trans*)rcv_buf;
        ui_val = mesg->uiCmdMsg;
#ifdef _DEBUG
        if ((ui_val != NETNDT) && (ui_val != GETIMAGE)) {
            for (int c = 0; c < rcvlen; c++) {
#ifndef _WIN32
                if (ui_val == 0) {
                    g_printed = 0;
                    fprintf(stdout, "\r");
                }
                if (c < 8) {
                    if (c == 1) {
                        fprintf(stdout, "[%x] ", rcv_buf[1]);
                    } else {
                        LeaveCriticalSection(&wrcon);
                        continue;
                    }
                }
                if (ui_val != 0)
#endif
                    fprintf(stdout, "%c", (unsigned char)rcv_buf[c]);
            }
#ifndef _WIN32
            if (ui_val > 15)
                ui_val = GETIMAGE;
            if (mesg->uiCmdMsg != 0 && mesg->uiCmdMsg < 16)
#endif
                fprintf(stdout, "\n");
            SetRecvState(RCV_TCP);
        } else {
            SetRecvState(RCV_ERR);
            if (g_printed >= 0) {
#ifdef _WIN32
                fprintf(stdout, "\rReceiving...");
#else
                fprintf(stdout, "\r...");
#endif
            } else {
                fprintf(stdout, "\r%*c\rReceiving...\n%s", 64, ' ', youSaid);
            }
        }
#endif
        if (ui_val == PEER2P) {
            int ip = atoi((const char*)mesg->peerIP);
            unsigned char* val = (unsigned char*)&ip;
            int port = atoi((const char*)mesg->peer_port);
            fprintf(stdout, "User:\t%s\nIP:\t%u.%u.%u.%u\nPORT:\t%d\n",
                mesg->username, val[3], val[2], val[1], val[0], port);
            if (port <= 0) {
                fprintf(stdout, "Error number of user port, stopping send P2P message!\n");
            } else {
                struct MoreMesg p2pmsg;
                memset(&p2pmsg, 0, sizeof(MoreMesg));
                p2pmsg.cmd[0] = mesg->uiCmdMsg;
                memcpy(&p2pmsg.mesg, "hello", 6);
                int parse = p2pMessage(mesg->username, ip, port, (char*)&p2pmsg);
                if (parse == 0) {
                    SetRecvState(RCV_P2P);
                }
            }
        }
        if (ui_val == NETNDT) {
            if (atoi((const char*)mesg->recv_mesg.status) == 200) {
                fprintf(stdout, "\r%*c\rReceived: %s\n", 64, ' ', mesg->recv_mesg.message);
                SetRecvState(RCV_NDT);
                if (g_printed > 0) {
                    fprintf(stdout, youSaid);
                }
            } else if (rcvlen < 0) {
                fprintf(stdout, "Status error.\n");
                closesocket(client->sock);
                LeaveCriticalSection(&wrcon);
                exit(0);
            } else {
                fprintf(stdout, "\r%*c\r%s\n", 64, ' ', mesg->ndt_msg);
                SetRecvState(RCV_TCP);
                g_printed = 2;
            }
        }
        if (ui_val == GETIMAGE) {
            int ndt_len = atoi(rcv_buf + 22);
            FILE* file = NULL;
            if (count == 0) {
                file = fopen(filename, "w");
                if (file == NULL)
                    continue;
                fclose(file);
                count = 1;
            }
            file = fopen(filename, "ab+");
            if (file == NULL)
                continue;
            if (fw_len == ndt_len) {
                SetRecvState(RCV_SCC);
                fprintf(stdout, "\n");
                fclose(file);
                LeaveCriticalSection(&wrcon);
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
            MessageBox(0, srv_net, title, MB_OK);
            SetChatActive(rcvlen);
            closesocket(client->sock);
            exit(0);
        };
    };
};

st_trans* ParseChatMesg(st_trans& trans)
{
    memset(&trans.user_sign, 0, 24);
    switch (trans.uiCmdMsg) {
    case REGISTER:
        fprintf(stdout, "Input a new username AND password to register, divide with BLANK(' ').\n");
        scanf_s("%s %s", trans.username, 24, trans.password, 24);
        break;
    case LOGIN:
    {
        if (trans.username[0] != '\0') {
            static char title[64];
            sprintf(title, "Welcome %s", (char*)trans.username);
            SetConsoleTitle(title);
        } else {
            fprintf(stdout, "Input username AND password to login, divide with BLANK(' ').\n");
            scanf_s("%s %s", trans.username, 24, trans.password, 24);
        }
        break;
    }
    case SETPSW:
    {
        gets_s((char*)trans.password, 24);
        break;
    }
    case PEER2P:
    case NETNDT:
    {
        memcpy(trans.type, "NDT", 4);
        memcpy(&trans.peer_name, "iv9527", 7);
        trans.more_mesg.cmd[0] = NETNDT;
#ifdef NDT_ONLY
        if (g_printed > -1) {
            fprintf(stdout, youSaid);
            g_printed = 1;
        }
#else
        if (g_printed != 2) {
            fprintf(stdout, "Set chatting message, limit on 16 characters.\n");
            g_printed = 1;
        }
#endif
        memset(&trans.more_mesg.mesg, 0, 16);
        if (scanf_s("%s", &trans.more_mesg.mesg.message, 16) <= 0) {
            fprintf(stdout, "Error: characters limit 16!\n");
            g_printed = trans.uiCmdMsg = -1;
            return &trans;
        }
        break;
    }
    case USERGROUP:
    {
        fprintf(stdout, "Input group name to show members in: ");
        scanf_s("%s", trans.group_name, (unsigned)_countof(trans.group_name));
        break;
    }
    case HOSTGROUP:
    {
        fprintf(stdout, "Input group name AND group mark be to host, divide with BLANK(' ').\n");
        scanf_s("%s %s", trans.group_host, (unsigned)_countof(trans.group_host), (trans.group_mark), (unsigned)_countof(trans.group_mark));
        break;
    }
    case JOINGROUP:
    {
        fprintf(stdout, "Input group name you want to join in: ");
        scanf_s("%s", (trans.group_join), (unsigned)_countof(trans.group_join));
        break;
    }
    default:
    {
        if (trans.value[0] == 0x0) {
            MessageBox(0, const_cast<char*>("LoggingIn failure."), const_cast<char*>("message"), MB_OK);
            return &trans;
        }
    }
    }
    return &trans;
}

bool is_ip_valid(char s[])
{
    int val = 0, dot = 0;
    size_t len = strlen(s);
    if (s[0] == '.')
        return false;
    if (s[0] <= '9' && s[0] >= '0')
        val = val + s[0] - '0';
    else
        return false;
    for (size_t i = 1; i < len; i++) {
        if ((s[i] > '9' || s[i] < '0') && s[i] != '.')
            return false;
        if (s[i] == '.' && s[i - 1] == '.')
            return false;
        if (s[i] == '.') {
            val = 0; dot++;
        } else {
            val = val * 10 + s[i] - '0';
            if (val > 255)
                return false;
        }
    }
    if (dot == 3)
        return true;
    else
        return false;
}

int main(int argc, char* argv[])
{
    st_sock sock = {};
    memset(&sock, 0, sizeof(st_sock));
    if (argc > 1) {
        if (!is_ip_valid(argv[1]))
            fprintf(stdout, "Error: '%s' not in correct IP format.\n", argv[1]);
        else
            memcpy(sock.IP, argv[1], strlen(argv[1]) + 1);
    }
    if (StartChat(InitChat(&sock), parseRcvMsg) < 0)
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
        volatile int received = GetRecvState();
        g_printed = 0;
#ifdef USR_TEST
        memcpy(msg.username, "AAAAA", 6);
        memcpy(msg.password, "AAAAA", 6);
#endif
#ifdef NDT_ONLY
        if (comm > 1) {
            comm = CHATWITH;
        }
        msg.more_mesg.cmd[0] = msg.uiCmdMsg = comm;
        while (IsChatActive() == 0) { ; }
        comm++;
#else
        int comm = 0;
        if (received >= RCV_SCC) {
            fprintf(stdout, "Input command [1-13]: ");
            if (scanf("%3d", &comm) <= 0) {
                fprintf(stdout, "Command format to integer error .\n");
                break;
            }
            if (comm > 13 || comm < 0) {
                fprintf(stdout, "Error cmd value: out of range [1,13].\n");
                continue;
            }
        }
        msg.uiCmdMsg = comm;
#endif // NDT_ONLY
        if (received == RCV_NDT) {
            msg.more_mesg.cmd[0] = msg.uiCmdMsg = NETNDT;
        }
        if (received > RCV_ERR) {
            SetRecvState(RCV_ERR);
            if (SendClientMessage(ParseChatMesg(msg)) < 0) {
                fprintf(stdout, "Error while set chatting message.\n");
                return -1;
            }
        }
        SLEEP(100);
    }
    return 0;
}
