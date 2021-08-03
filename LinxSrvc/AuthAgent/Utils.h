#include <sstream>
#include <string>
#include <vector>

inline bool isNum(const std::string& str)
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

static std::vector<std::string> parseUri(const std::string& uri)
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
            if ((ssize_t)src.find("/") < 0) {
                vec.emplace_back(src);
            }
            len = src.size();
            i = 0;
        }
    }
    return vec;
}