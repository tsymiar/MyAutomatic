#pragma once
#include <mutex>
#include <string>
#include <vector>

extern "C" {
    class KaSocket
    {
    public:
        KaSocket() {};
        KaSocket(unsigned short servport);
        KaSocket(const char* srvip, unsigned short servport);
        int start();
        int connect();
        int send(const char* data, int len);
        int broadcast(const char* data, int len);
        int recv(char* buff, int len);
        bool running();
        void wait(unsigned int tms);
        void setCallback(void(*func)(void*));
        void addCallback(void(*func)(void*));
        virtual ~KaSocket();
    private:
        struct Head {
            char resv;
            volatile unsigned long long ssid; //ssid = port | socket | ip 
            int flag;
        };
        struct Network {
            int socket;
            std::string IP;
            unsigned short PORT;
            Head head;
            volatile bool run_ = false;
        } network; // current connect
        bool server = false;
        std::mutex mtxlck;
        std::vector<Network> networks;
        std::vector<void(*)(void*)> callbacks;
        void notify(int socket);
        void runCallback(KaSocket* sock, void(*func)(void*));
        void heartBeat(Network& network);
        unsigned long long setSsid(Network network, int socket);
        bool verifySsid(Network network, unsigned long long ssid);
    };
}
