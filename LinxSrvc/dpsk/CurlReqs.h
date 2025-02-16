#include <curl/curl.h>
#include <string>
#include <vector>

static std::vector<std::string> Models = {
    "deepseek-chat",
    "deepseek-coder",
    "deepseek-reasoner",
    "deep_thought_v1",
    "deepseek/deepseek-r1:free"
};

struct ReqsPara {
    bool json;
    bool multi;
    bool balance;
    struct ApiPara {
        bool stream;
        float temperature;
        int max_tokens;
        float top;
        bool web_search = false;
        std::string model = Models[2];
        std::string depth = "high";
        std::string system_msg = "You are a helpful assistant.";
        ApiPara() : stream(false), temperature(0.7), max_tokens(500), top(0.9) { }
    };
    ApiPara apiPara;
    ReqsPara() : json(true), multi(true), balance(false) { }
};

class CurlReqs {
public:
    CurlReqs();
    ~CurlReqs();

    bool initReqs();
    bool performRequest(const std::string& url, std::string& response);
    void setHeader(const std::string& header);
    void setPostFields(const char* data, bool json = true);

    static std::string processChat(const std::string& text, const ReqsPara& para = ReqsPara());
    static std::string getBalance();

public:
    static std::vector<std::string> m_messages;

private:
    CURL* m_curl = nullptr;
    struct curl_slist* m_headers = nullptr;
private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};
