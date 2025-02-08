#include "CurlReq.h"
#include <iostream>
#include <thread>

CurlReq::CurlReq() : m_curl(curl_easy_init())
{ }

CurlReq::~CurlReq()
{
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

bool CurlReq::init()
{
    if (!m_curl) {
        m_curl = curl_easy_init();
    }
    return m_curl != nullptr;
}

bool CurlReq::performRequest(const std::string& url, std::string& response)
{
    if (!m_curl) {
        return false;
    }

    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);

    CURLcode res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(m_headers);
        return false;
    }

    curl_slist_free_all(m_headers);
    return true;
}

void CurlReq::setHeader(const std::string& header)
{
    m_headers = curl_slist_append(m_headers, header.c_str());
}

void CurlReq::setPostData(const char* data, bool isJson)
{
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, data);
    if (isJson) {
        m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
    } else {
        m_headers = curl_slist_append(m_headers, "Content-Type: application/x-www-form-urlencoded");
    }
}

size_t CurlReq::writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

#include <cstdlib> 
#include <nlohmann/json.hpp>
using json = nlohmann::json;

void showLoadingIndicator(bool& isRunning)
{
    const char* loadingSymbols = "|/-\\";
    int index = 0;
    while (isRunning) {
        std::cout << "\r" << loadingSymbols[index++ % 4] << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\r " << std::flush;
}

std::string CurlReq::processInput(const std::string& input)
{
    CurlReq curl;
    if (!curl.init()) {
        std::cerr << "CURL Not init!" << std::endl;
        return "";
    }

    const char* env_value = std::getenv("DPSK_API_KEY");
    if (env_value != nullptr) {
        std::cout << "DPSK_API_KEY: " << env_value << std::endl;
    } else {
        std::cout << "DPSK_API_KEY environment variable not found." << std::endl;
        return "";
    }

    std::string key = env_value;
    curl.setHeader("Authorization: Bearer " + key);

    std::string postData = "{\"model\": \"deepseek-chat\",\"messages\" : [{\"role\": \"system\", \"content\" : \"You are a helpful assistant.\"},{ \"role\": \"user\", \"content\" : \"" + input + "\" }] ,\"stream\" : false}";
    curl.setPostData(postData.c_str());

    bool isRunning = true;
    std::thread loadingThread(showLoadingIndicator, std::ref(isRunning));

    std::string message;
    if (curl.performRequest("https://api.deepseek.com/chat/completions", message)) {
        isRunning = false;
        loadingThread.join();
        // std::cout << "\r" << message << std::endl;
    } else {
        isRunning = false;
        loadingThread.join();
        std::cerr << "Request Not ok!" << std::endl;
        return "";
    }

    std::string content = "";
    try {
        auto jsonResponse = json::parse(message);
        content = jsonResponse["choices"][0]["message"]["content"];
    } catch (json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
    return ("\r--------\n" + content + "\n--------");
}
