#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include <functional>
// extern "C" {
class KaiSocket {
public:
    KaiSocket() {};
    KaiSocket(unsigned short servport);
    KaiSocket(const char* srvip, unsigned short servport);
    virtual ~KaiSocket();
    KaiSocket& GetInstance();
    int start();
    int connect();
    int send(const char* data, int len);
    int broadcast(const char* data, int len);
    int broker(char* data, int len);
    int recv(char* buff, int len);
    bool running();
    static void wait(unsigned int tms);
    void registerCallback(int(*func)(KaiSocket*));
    void appendCallback(int(*func)(KaiSocket*));
    void appendCallback(std::function<int(KaiSocket*)>);
    void setResponseHandle(void(*func)(char*, int), char*, int&);
    void setRequestHandle(void(*func)(char*, int), char*, int&);
    enum KaiRoles {
        Producer = 1,
        Consumer,
        Pubilsher,
        Subscriber
    };
    // call after connect()
    int ProduceClient(const std::string& body, ...);
    int ConsumeClient();
    inline void SetTopic(const std::string& topic, int tag = Pubilsher)
    {
        memcpy(m_network.flag.mqid, topic.c_str(), 32);
        m_network.flag.etag = tag;
    };
    struct SharedKaiSocket : std::enable_shared_from_this<KaiSocket> {
        std::shared_ptr<KaiSocket> GetSharedInstance()
        {
            return shared_from_this();
        }
    };
    template<typename T, typename... Args>
    static std::shared_ptr<T> GetSharedInstance(Args&&... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
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
    } m_network;
    bool m_isClient = false;
    volatile unsigned int g_threadNo_ = 0;
    std::vector<Network> m_networks{};
    std::vector<int(*)(KaiSocket*)> m_callbacks{};
    void(*submit)(char*, int) = nullptr;
    void(*receive)(char*, int) = nullptr;
    void handleNotify(int socket);
    void runCallback(KaiSocket* sock, int (*func)(KaiSocket*));
    unsigned long long setSsid(const Network& network, int socket);
    bool verifySsid(int key, unsigned long long ssid);
    struct Message {
        Header head;
        struct Payload {
            char stat[8];
            char body[256];
        } data;
    };
    int produce(const Message& msg);
    int consume(Message& msg);
    std::deque<Message*>* msgque = new std::deque<Message*>();
};
// }
