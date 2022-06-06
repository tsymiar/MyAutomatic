#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include <functional>

enum KaiRoles {
    USAGE = -1,
    NONE = 0,
    PRODUCER,
    CONSUMER,
    SERVER,
    BROKER,
    CLIENT,
    PUBLISH,
    SUBSCRIBE,
    FILE_CONTENT
};

#ifdef _WIN32
#define WINDOWS_IGNORE_PACKING_MISMATCH
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#define packed
#define __attribute__(a)
typedef int ssize_t;
#pragma comment(lib, "WS2_32.lib")
#include <WinSock2.h>
#else
using SOCKET = int;
#ifdef __linux__
#define USE_EPOLL
#endif
#endif

class KaiSocket {
public:
#pragma pack(1)
    struct Header {
        char rsv;
        int etag;
        /*volatile*/ unsigned long long ssid; // ssid = port | socket | ip
        char buffer[32];
        unsigned int size;
    } __attribute__((packed));
#pragma pack()
#pragma pack(1)
    struct Message {
        Header head{};
        struct Payload {
            char stat[8];
            char body[0]; // [256]
        } __attribute__((packed)) data {};
        void* operator new(size_t, const Message& msg) {
            static void* mss = (void*)(&msg);
            return mss;
        }
    } __attribute__((packed));
#pragma pack()
    typedef int(*KAISOCKHOOK)(KaiSocket*);
    typedef void(*RECVCALLBACK)(const Message&);
    static char G_KaiRole[][0xe];
    KaiSocket() = default;
    virtual ~KaiSocket() = default;
public:
#ifdef FULLY_COMPILE
    explicit KaiSocket(unsigned short lsn_prt);
    KaiSocket(const char* ip, unsigned short port);
#else
    int Initialize(unsigned short lsn_prt);
#endif
    int Initialize(const char* ip, unsigned short port);
    static KaiSocket& GetInstance();
    // workflow
    int start();
    int connect();
    //
    int Broker();
    ssize_t Publisher(const std::string& topic, const std::string& payload, ...);
    ssize_t Subscriber(const std::string& message, RECVCALLBACK callback = nullptr);
    // packaged
    static void wait(unsigned int tms);
    ssize_t send(const uint8_t* data, size_t len);
    ssize_t recv(uint8_t* buff, size_t size);
    ssize_t broadcast(const uint8_t* data, size_t len);
    // callback
    void registerCallback(KAISOCKHOOK func);
    void appendCallback(KAISOCKHOOK func);
    //
    static bool isLittleEndian();
    std::string getFile2string(const std::string&);
#ifdef FULLY_COMPILE
    void appendCallback(const std::function<int(KaiSocket*)>&);
    void setResponseHandle(void(*func)(uint8_t*, size_t), uint8_t*, size_t&);
    void setRequestHandle(void(*func)(uint8_t*, size_t), uint8_t*, size_t&);
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
#endif
    // private members should be deleted in release version head-file
private:
    struct Network {
        SOCKET socket;
        std::string IP;
        unsigned short PORT;
        volatile bool run_01 = false;
        bool client = false;
        Header flag;
    } m_network;
    std::mutex m_lock = {};
    std::vector<Network> m_networks{};
    std::vector<int(*)(KaiSocket*)> m_callbacks{};
    static std::deque<const Message*>* m_msgQue;
private:
    uint64_t setSsid(const Network& network, SOCKET socket = 0);
    ssize_t writes(Network, const uint8_t*, size_t);
    bool checkSsid(SOCKET key, uint64_t ssid);
    bool running();
    void finish();
    void handleNotify(Network& network);
    void runCallback(KaiSocket* sock, KAISOCKHOOK func);
    void setTopic(const std::string& topic, Header& header);
    int produce(const Message& msg);
    ssize_t consume(Message& msg);
public:
    void rsync(Message& msg);
};
