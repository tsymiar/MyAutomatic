#include "Utils.h"

#include <algorithm>
#include <sstream>
#include <fstream>
#include <streambuf>

bool isNum(const std::string& in)
{
    std::stringstream sin(in);
    double d;
    char c;
    if (!(sin >> d)) {
        return false;
    }
    if (sin >> c) {
        return false;
    }
    return true;
}

bool ipIsValid(const char* ip)
{
    int value = 0;
    int dot = 0;
    char last = '.';

    while (*ip) {
        if (*ip == '.') {
            dot++;
            if (dot > 3) {
                return false;
            }
            if (value >= 0 && value <= 255) {
                value = 0;
            } else {
                return false;
            }
        } else if (*ip >= '0' && *ip <= '9') {
            value = value * 10 + *ip - '0';
            if (last == '.' && *ip == '0') {
                return false;
            }
        } else {
            return false;
        }
        last = *ip;
        ip++;
    }

    if (value >= 0 && value <= 255) {
        if (3 == dot) {
            return true;
        }
    }
    return false;
}

long sIP2long(const char* ip)
{
    if (ip == nullptr) {
        return 0;
    }
    long lip = 0;
    long ip1, ip2, ip3, ip4;
    if (sscanf(ip, "%ld.%ld.%ld.%ld", &ip1, &ip2, &ip3, &ip4) == 4) {
        lip = (ip1 << 24) + (ip2 << 16) + (ip3 << 8) + ip4;
    }
    return lip;
}

std::vector<std::string> parseUri(const std::string& uri)
{
    std::vector<std::string> vec{};
    size_t end = uri.find("?");
    std::string src = uri.substr(0, end);
    size_t len = src.size();
    for (size_t i = 0; i < len; i++) {
        if (src[i] == '/') {
            std::string action = src.substr(0, i);
            if (!action.empty()) {
                vec.emplace_back(action);
            }
            src = src.substr(i + 1, len);
            if (src.find("/") == std::string::npos) {
                vec.emplace_back(src);
            }
            len = src.size();
            i = 0;
        }
    }
    return vec;
}

std::string getVariable(const std::string& url, const std::string& key)
{
    std::string val = {};
    size_t pos = url.find(key);
    if (pos != std::string::npos) {
        val = url.substr(pos, url.size());
        pos = val.find("=");
        size_t org = val.find("&");
        if (org == std::string::npos) {
            val = val.substr(pos + 1, val.size() - pos - 1);
        } else {
            val = val.substr(pos + 1, org - pos - 1);
        }
    }
    return val;
}

std::string getFile2string(const std::string& filename)
{
    std::string content{};
    std::ifstream file(filename);
    if (file.is_open()) {
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
    file.close();
    return content;
}
