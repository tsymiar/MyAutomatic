#include <curl/curl.h>
#include <string>

class CurlReq {
public:
    CurlReq();
    ~CurlReq();

    bool init();
    bool performRequest(const std::string& url, std::string& response);
    void setHeader(const std::string& header);
    void setPostData(const char* data, bool isJson = true);
    static std::string processInput(const std::string& input);

private:
    CURL* m_curl = nullptr;
    struct curl_slist* m_headers = nullptr;
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};
