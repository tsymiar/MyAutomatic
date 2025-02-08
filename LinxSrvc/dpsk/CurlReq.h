#include <curl/curl.h>
#include <string>
#include <vector>

class CurlReq {
public:
    CurlReq();
    ~CurlReq();

    bool init();
    bool performRequest(const std::string& url, std::string& response);
    void setHeader(const std::string& header);
    void setPostFields(const char* data, bool json = true);

    static std::string processChat(const std::string& text, bool multi = false, bool balance = false);
    static std::string getBalance();

public:
    static std::vector<std::string> m_messages;

private:
    CURL* m_curl = nullptr;
    struct curl_slist* m_headers = nullptr;
private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};
