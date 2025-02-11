#include "CurlReqs.h"
#include <iostream>
#include <thread>
#include <atomic>

std::vector<std::string> CurlReqs::m_messages = std::vector<std::string>();

CurlReqs::CurlReqs() : m_curl(curl_easy_init()), m_headers(nullptr)
{ }

CurlReqs::~CurlReqs()
{
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
    if (m_headers) {
        curl_slist_free_all(m_headers);
    }
}

bool CurlReqs::initReqs()
{
    if (!m_curl) {
        m_curl = curl_easy_init();
    }
    return m_curl != nullptr;
}

bool CurlReqs::performRequest(const std::string& url, std::string& response)
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
        return false;
    }

    return true;
}

void CurlReqs::setHeader(const std::string& header)
{
    m_headers = curl_slist_append(m_headers, header.c_str());
}

void CurlReqs::setPostFields(const char* data, bool json)
{
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, data);
    if (json) {
        m_headers = curl_slist_append(m_headers, "Content-Type: application/json");
    } else {
        m_headers = curl_slist_append(m_headers, "Content-Type: application/x-www-form-urlencoded");
    }
}

std::string CurlReqs::getBalance()
{
    ReqsPara para;
    para.multi = false;
    para.balance = true;
    std::string content = processChat("null", para);
    return ("\r--------\n" + content + "\n--------");
}

size_t CurlReqs::writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

#include <cstdlib> 
#include <nlohmann/json.hpp>
using json = nlohmann::json;

std::string combineMessage(const std::string& msg, ReqsPara::ApiPara para)
{
    CurlReqs::m_messages.push_back(msg);
    json js_data;
    js_data["model"] = para.model;
    js_data["stream"] = para.stream;
    js_data["temperature"] = para.temperature;
    js_data["max_tokens"] = para.max_tokens;
    js_data["search_config"]["enable_web_search"] = para.web_search;
    js_data["parameters"]["depth"] = para.depth;
    js_data["top_p"] = para.top;
    js_data["messages"][0] = { {"role", "system"}, {"content", para.system_msg} };
    for (size_t i = 0; i < CurlReqs::m_messages.size(); i++) {
        json js_msg;
        js_msg["content"] = CurlReqs::m_messages[i];
        js_msg["role"] = "user";
        js_data["messages"].push_back(js_msg);
    }
    std::string message = js_data.dump();
    // std::cout << "json: " << message << std::endl;
    return message;
}

void showLoadingIndicator(std::atomic<bool>& isRunning)
{
    const char* loadingSymbols = "|/-\\";
    int index = 0;
    while (isRunning) {
        std::cout << "\r" << loadingSymbols[index++ % 4] << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\r " << std::flush;
}

std::string CurlReqs::processChat(const std::string& text, const ReqsPara& para)
{
    CurlReqs reqs;
    if (!reqs.initReqs()) {
        std::cerr << "CURL Not init reqs!" << std::endl;
        return std::string();
    }

    const char* env_value = std::getenv("DPSK_API_KEY");
    if (env_value != nullptr) {
        // std::cout << "DPSK_API_KEY: " << env_value << std::endl;
    } else {
        std::cout << "DPSK_API_KEY environment variable not found." << std::endl;
        return std::string();
    }

    std::string key = env_value;
    reqs.setHeader("Authorization: Bearer " + key);

    std::string postData = "{\"model\": \"deepseek-chat\",\"messages\" : [{\"role\": \"system\", \"content\" : \"You are a helpful assistant.\"},{ \"role\": \"user\", \"content\" : \"" + text + "\" }] ,\"stream\" : false}";
    if (para.multi) {
        postData = combineMessage(text, para.apiPara);
    }
    reqs.setPostFields(postData.c_str());

    std::atomic<bool> isRunning(true);
    std::thread loadingThread(showLoadingIndicator, std::ref(isRunning));

    std::string uri = "https://api.deepseek.com/chat/completions";
    if (para.balance) {
        uri = "https://api.deepseek.com/chat/balance";
    }
    std::string message;
    if (reqs.performRequest(uri, message)) {
        isRunning = false;
        loadingThread.join();
        // std::cout << "\r" << message << std::endl;
    } else {
        isRunning = false;
        loadingThread.join();
        std::cerr << "Request Not ok!" << std::endl;
        return std::string();
    }

    std::string content = "";
    try {
        auto jsonResponse = json::parse(message);
        if (para.balance) {
            return (jsonResponse["error_msg"].empty() ? jsonResponse["balance"].dump() : jsonResponse["error_msg"].dump());
        } else {
            if (!jsonResponse["error"].empty()) {
                return jsonResponse["error"]["message"].dump();
            }
        }
        content = jsonResponse["choices"][0]["message"]["content"];
    } catch (const std::exception& e) {
        std::cerr << "JSON parse exception: " << e.what() << std::endl;
        return std::string();
    }
    return ("\r--------\n" + content + "\n--------");
}
