#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <mutex>

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
        int transfer(char* data, int len);
        int recv(char* buff, int len);
        bool running();
        static void wait(unsigned int tms);
        void setCallback(void(*func)(void*));
        void addCallback(void(*func)(void*));
        // call after connect()
        inline void setTopic(std::string topic) { 
            memcpy(current.flag.mqid, topic.c_str(), 32);
        };
        virtual ~KaSocket();
    private:
        struct Header {
            char resv;
            int etag;
            volatile unsigned long long ssid; //ssid = port | socket | ip
            char mqid[32]; // topic
            int size;
        };
        struct Network {
            int socket;
            std::string IP;
            unsigned short PORT;
            volatile bool run_ = false;
            Header flag;
        } current;
        bool server = false;
        volatile unsigned int g_threadNo_ = 0;
        std::vector<Network> networks;
        std::vector<void(*)(void*)> callbacks;
        void handleNotify(int socket);
        void runCallback(KaSocket* sock, void(*func)(void*));
        unsigned long long setSsid(Network network, int socket);
        bool verifySsid(Network network, unsigned long long ssid);
    public:
        struct Message {
            Header head;
            struct Payload {
                char result[32];
                char body[256];
            } data;
        };
        int produceClient(std::string body, ...);
        int consumeClient();
    private:
        int produce(Message& msg);
        int consume(Message& msg);
        std::deque<Message*> *msgque = new std::deque<Message*>();
    };
}
