#include "Utils.h"

#include <algorithm>
#include <sstream>

bool isNum(const std::string& str)
{
    std::stringstream sin(str);
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
