#include "KaiSocket.h"

#ifndef _WIN32
#include <unistd.h>
#endif
#include <iostream>
#include <fstream>

using namespace std;

int hook_rcv(KaiSocket*);
int hook_snd(KaiSocket*);

std::string getFileVariable(const std::string& filename, const std::string& keyword);

int main(int argc, char* argv[])
{
    KaiRoles usage = USAGE;
    if (argc > 1) {
        string argv1 = string(argv[1]);
        usage = (argv1 == "-C" ? CLIENT :
            (argv1 == "-S" ? SUBSCRIBE :
                (argv1 == "-P" ? PUBLISH :
                    (argv1 == "-TF" ? FILE_CONTENT :
                        (argv1 == "-B" ? BROKER : SERVER)))));
    }
#ifdef _USE_FORK_PROCESS_
    pid_t child = fork();
    if (child == 0) {
#endif
        KaiSocket kai;
        const int PORT = 9999;
        const char* IP = "127.0.0.1";
        string var = getFileVariable("kaics.cfg", "IP");
        if (!var.empty()) {
            IP = var.c_str();
        }
        if (usage >= CLIENT) {
            kai.Initialize(IP, PORT);
        } else {
            kai.Initialize(nullptr, PORT);
        }
        if (usage == CLIENT || usage == SERVER) {
            kai.registerCallback(hook_rcv);
            kai.appendCallback(hook_snd);
        }
        if (usage < 0xe && usage >= 0) {
            cout << argv[0] << ": Run as [" << usage << "](" << KaiSocket::G_KaiRole[usage] << ")" << endl;
        }
        string topic = "topic";
        string param = "a123+/";
        if (argc > 2) {
            topic = string(argv[2]);
        }
        if (argc > 3) {
            param = string(argv[3]);
        }
        switch (usage) {
        case USAGE:
            cout << "Usage:" << endl
                << "-A -- run as server" << endl
                << "-B -- run as broker" << endl
                << "-C -- run as client" << endl
                << "<none> -- print this message" << endl
                << "-S  [topic] -- run as subscriber, default topic is 'topic'" << endl
                << "-P  [topic] [payload] -- run as publisher messaging to broker" << endl
                << "-TF [topic] [filename] -- run as publisher trans file content" << endl;
            break;
        case CLIENT:
            kai.connect();
            break;
        case SERVER:
            kai.start();
            break;
        case BROKER:
            kai.Broker();
            break;
        case SUBSCRIBE:
            kai.Subscriber(topic);
            break;
        case PUBLISH:
            kai.Publisher(topic, param);
            break;
        case FILE_CONTENT:
            param = kai.getFile2string(param);
            if (!param.empty()) {
                kai.Publisher(topic, param);
            }
            break;
        default:
            break;
        }
#ifdef _USE_FORK_PROCESS_
    } else if (child > 0) {
        cout << "child process " << child << " started" << endl;
    } else {
        cout << "KaiSocket fork process failed!" << endl;
    }
#endif
}

int hook_rcv(KaiSocket* kai)
{
    uint8_t* text = new uint8_t[64];
    memset(text, 0, 64);
    int len = kai->recv(text, 64);
    if (len > 0) {
        for (int c = 0; c < len; c++) {
            if (c > 0 && c % 32 == 0)
                fprintf(stdout, "\n");
            fprintf(stdout, "%02x ", static_cast<unsigned char>(text[c]));
        }
        fprintf(stdout, "\n");
    }
    delete[]text;
    return 0;
}

int hook_snd(KaiSocket* kai)
{
    auto* text = new uint8_t[64];
    memset(text, 0, 64);
    char c;
    (cin >> text).get(c);
    kai->send(text, 64);
    cout << "----------------" << endl;
    delete[]text;
    return 0;
}

std::string getFileVariable(const std::string& filename, const std::string& keyword)
{
    std::string content{};
    std::ifstream file(filename);
    if (file.is_open()) {
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    } else {
        return {};
    }
    file.close();
    if (content.empty()) {
        return {};
    }
    std::string val = {};
    size_t pos = content.find(keyword);
    if (pos != std::string::npos) {
        val = content.substr(pos, content.size());
        pos = val.find("=");
        size_t org = val.find("&");
        if (org == std::string::npos) {
            val = val.substr(pos + 1, val.size() - pos - 1);
        } else {
            val = val.substr(pos + 1, org - pos - 1);
        }
    }
    if (val.back() == '\n') {
        val.pop_back();
    }
    return val;
}
