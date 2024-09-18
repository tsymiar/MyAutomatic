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
    CRITICAL_SECTION wrcon;
    InitializeCriticalSection(&wrcon);
    StClient* client = (StClient*)lprcv;
    int fw_len = 0;
    int ui_val = 0;
    volatile int count = 0;
    static char rcv_buf[256];
    while (1) {
        if (client->flag == 0)
            continue;
        memset(rcv_buf, 0, 256);
        int size = recv(client->sock, rcv_buf, 256, 0);
        if (size == 2 && (rcv_buf[1] == '\0')) {
            Sleep(1);
            continue;
        }
        EnterCriticalSection(&wrcon);
        StMsgContent* msg = (StMsgContent*)rcv_buf;
        ui_val = msg->uiCmdMsg;
#ifdef _DEBUG
        if ((ui_val != NETNDT) && (ui_val != GETIMAGE)) {
            for (int c = 0; c < size; c++) {
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
            if (msg->uiCmdMsg != 0 && msg->uiCmdMsg < 16)
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
            int ip = atoi((const char*)msg->peerIP);
            unsigned char* val = (unsigned char*)&ip;
            int port = atoi((const char*)msg->peer_port);
            fprintf(stdout, "User:\t%s\nIP:\t%u.%u.%u.%u\nPORT:\t%d\n",
                msg->username, val[3], val[2], val[1], val[0], port);
            if (port <= 0) {
                fprintf(stdout, "Error number of user port, stopping send P2P message!\n");
            } else {
                struct ExtraMessage p2pMsg;
                memset(&p2pMsg, 0, sizeof(ExtraMessage));
                p2pMsg.cmd[0] = msg->uiCmdMsg;
                memcpy(&p2pMsg.msg, "hello", 6);
                int parse = p2pMessage(msg->username, ip, port, (char*)&p2pMsg);
                if (parse == 0) {
                    SetRecvState(RCV_P2P);
                }
            }
        }
        if (ui_val == NETNDT) {
            if (atoi((const char*)msg->rcv_msg.status) == 200) {
                fprintf(stdout, "\r%*c\rReceived: %s\n", 64, ' ', msg->rcv_msg.message);
                SetRecvState(RCV_NDT);
                if (g_printed > 0) {
                    fprintf(stdout, youSaid);
                }
            } else if (size < 0) {
                fprintf(stdout, "Status error.\n");
                closesocket(client->sock);
                LeaveCriticalSection(&wrcon);
#ifdef _WIN32
                return;
#else
                exit(0);
#endif
            } else {
                fprintf(stdout, "\r%*c\r%s\n", 64, ' ', msg->ndt_msg);
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
        if (ui_val == 0xf) {
            SetRecvState(RCV_SCC);
        }
        LeaveCriticalSection(&wrcon);
        if (size <= 0 || (msg->value[0] == 'e' && msg->value[1] == '8')) {
            char detail[64];
            snprintf(detail, 38 + 16, "Connection lost!\n\tsocket: %s, size=%d", _itoa((int)client->sock, (char*)msg, 10), size);
            char title[32];
            snprintf(title, 32, "%s:%d", inet_ntoa(client->srvaddr.sin_addr), client->srvaddr.sin_port);
            MessageBox(0, detail, title, MB_OK);
            SetChatActive(size);
            closesocket(client->sock);
#ifdef _WIN32
            return;
#else
            exit(0);
#endif
        };
    };
};

StMsgContent* ParseChatData(StMsgContent& content)
{
    memset(&content.user_sign, 0, 24);
    switch (content.uiCmdMsg) {
    case REGISTER:
        fprintf(stdout, "Type in a new username AND password to register, divide with BLANK(' ').\n");
        scanf_s("%s %s", content.username, 24, content.password, 24);
        break;
    case LOGIN:
    {
        if (content.username[0] != '\0') {
            static char title[36];
            snprintf(title, 36, "Welcome %s", reinterpret_cast<char*>(content.username));
            SetConsoleTitle(title);
        } else {
            fprintf(stdout, "Type in username AND password to login, divide with BLANK(' ').\n");
            scanf_s("%s %s", content.username, 24, content.password, 24);
        }
        break;
    }
    case SETPSW:
    {
        gets_s((char*)content.password, 24);
        break;
    }
    case PEER2P:
    case NETNDT:
    {
        memcpy(content.type, "NDT", 4);
        memcpy(&content.peer_name, "iv9527", 7);
        content.ext_msg.cmd[0] = NETNDT;
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
        memset(&content.ext_msg.msg, 0, 16);
        if (scanf_s("%s", &content.ext_msg.msg.message, 16) <= 0) {
            fprintf(stdout, "Error: characters limit 16!\n");
            g_printed = content.uiCmdMsg = -1;
            return &content;
        }
        break;
    }
    case USERGROUP:
    {
        fprintf(stdout, "Type in group name to show members in: ");
        scanf_s("%s", content.group_name, (unsigned)_countof(content.group_name));
        break;
    }
    case HOSTGROUP:
    {
        fprintf(stdout, "Type in group name AND group mark be to host, divide with BLANK(' ').\n");
        scanf_s("%s %s", content.group_host, (unsigned)_countof(content.group_host), (content.group_mark), (unsigned)_countof(content.group_mark));
        break;
    }
    case JOINGROUP:
    {
        fprintf(stdout, "Type in group name you want to join in: ");
        scanf_s("%s", (content.group_join), (unsigned)_countof(content.group_join));
        break;
    }
    default:
    {
        if (content.value[0] == 0x0) {
            MessageBox(0, const_cast<char*>("content.value is null."), const_cast<char*>("message"), MB_OK);
            return &content;
        }
    }
    }
    return &content;
}

bool is_ip_valid(char s[])
{
    int val = 0, dot = 0;
    size_t len = strnlen(s, INET_ADDRSTRLEN);
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
    return (dot == 3);
}

int main(int argc, char* argv[])
{
    StSock sock = {};
    memset(&sock, 0, sizeof(StSock));
    if (argc > 1) {
        if (!is_ip_valid(argv[1]))
            fprintf(stdout, "Error: '%s' not in correct IP format.\n", argv[1]);
        else
            memcpy(sock.IP, argv[1], strnlen(argv[1], INET_ADDRSTRLEN) + 1);
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
        StMsgContent msg;
        memset(&msg, 0, sizeof(StMsgContent));
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
        msg.ext_msg.cmd[0] = msg.uiCmdMsg = comm;
        while (IsChatActive() == 0) { ; }
        comm++;
#else
        unsigned int comm = 0;
        if (received >= RCV_SCC) {
            fprintf(stdout, "Select a command [0x1 - 0xd] >: ");
            if (scanf("%x", &comm) <= 0) {
                fprintf(stdout, "Command not in integer format, exit.\n");
                break;
            }
            if (comm > 0xf || comm < 0) {
                fprintf(stdout, "Error cmd value: out of range [0x1,0xd].\n");
                continue;
            }
        }
        msg.uiCmdMsg = comm;
#endif // NDT_ONLY
        if (received == RCV_NDT) {
            msg.ext_msg.cmd[0] = msg.uiCmdMsg = NETNDT;
        }
        if (received > RCV_ERR) {
            SetRecvState(RCV_ERR);
            if (SendClientMessage(ParseChatData(msg)) < 0) {
                fprintf(stdout, "Error while set chatting message.\n");
                return -1;
            }
        }
        Sleep(100);
    }
    return 0;
}
