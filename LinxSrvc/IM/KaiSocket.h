#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include <functional>

class KaiSocket {
public:
    typedef int(*KAISOCKHOOK)(KaiSocket*);
    KaiSocket() = default;
    explicit KaiSocket(unsigned short lstnprt);
    KaiSocket(const char* srvip, unsigned short srvport);
    virtual ~KaiSocket() = default;
    static KaiSocket& GetInstance();
    // workflow
    int start();
    int connect();
    int send(const uint8_t* data, size_t len);
    int broadcast(const char* data, int len);
    int broker();
    int recv(char* buff, int size);
    static void wait(unsigned int tms);
    void finish();
    // callback
    void registerCallback(KAISOCKHOOK func);
    void appendCallback(KAISOCKHOOK func);
    void appendCallback(const std::function<int(KaiSocket*)>&);
    void setResponseHandle(void(*func)(uint8_t*, size_t), uint8_t*, size_t&);
    void setRequestHandle(void(*func)(uint8_t*, size_t), uint8_t*, size_t&);
    // after connect()
    void SetTopic(const std::string& topic, int tag = Producer);
    int Publisher(const std::string& topic, const std::string& payload, ...);
    ssize_t Subscriber(const std::string& message);
public:
    enum KaiRoles {
        Producer = 1,
        Consumer
    };
    struct Header {
        char rsv;
        int etag;
        volatile unsigned long long ssid; //ssid = port | socket | ip
        char topic[32];
        unsigned int size;
    };
    struct Message {
        Header head{};
        struct Payload {
            char stat[8];
            char body[0];
        } data{};
        void* operator new(size_t, const Message& msg) {
            return (void*)(&msg);
        }
    };
public:
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
    struct Network {
        int socket;
        std::string IP;
        unsigned short PORT;
        volatile bool run_ = false;
        Header flag;
    } m_network;
    bool m_isClient = false;
    volatile unsigned int m_threadNo_ = 0;
    std::mutex m_lock = {};
    std::vector<Network> m_networks{};
    std::vector<int(*)(KaiSocket*)> m_callbacks{};
    std::deque<Message*>* m_msgQue = new(std::nothrow)std::deque<Message*>();
    void(*submit)(char*, int) = nullptr;
    void(*receive)(char*, int) = nullptr;
private:
    uint64_t setSsid(const Network& network, int socket);
    void handleNotify(Network& network);
    void runCallback(KaiSocket* sock, KAISOCKHOOK func);
    bool verifySsid(int key, uint64_t ssid);
    bool running();
    int produce(const Message& msg);
    int consume(Message& msg);
};
