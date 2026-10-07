#pragma once
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <libgen.h>
#include <sstream>
#include <streambuf>
#include <string.h>
#include <string>
#include <vector>

#define Message(fmt, ...) ::fprintf(stdout, "\r[NOTICE](%s:%d)[%s]: " fmt "\n", basename(const_cast<char*>(__FILE__)),__LINE__,__FUNCTION__,##__VA_ARGS__)
#define Warning(fmt, ...) ::fprintf(stdout, "\r[WARN](%s:%d)[%s]: " fmt "\n", basename(const_cast<char*>(__FILE__)),__LINE__,__FUNCTION__,##__VA_ARGS__)
#define Error(fmt, ...) ::fprintf(stderr, "\r[ERROR](%s:%d)[%s]: " fmt "\n", basename(const_cast<char*>(__FILE__)),__LINE__,__FUNCTION__,##__VA_ARGS__)

inline bool isNum(const std::string& s)
{
    std::stringstream ss(s);
    double d;
    char c;
    if (!(ss >> d)) {
        return false;
    }
    if (ss >> c) {
        return false;
    }
    return true;
}

inline bool isIpAddr(const char* ip)
{
    int value = 0;
    int dots = 0;
    char last = '.';
    if (ip[0] == '.' || ip[0] == '0') {
        return false;
    }
    while (*ip) {
        if (*ip == '.') {
            dots++;
            if (dots > 3) {
                return false;
            }
            if (value >= 0 && value <= 0xff) {
                value = 0;
            } else {
                return false;
            }
        } else if (*ip >= '0' && *ip <= '9') {
            value = value * 10 + *ip - '0';
            if (last == '.' && *ip == '0'
                && *(ip + 1) != '\0' && *(ip + 2) == '0'
                && *(ip + 3) != '\0' && *(ip + 4) == '0') {
                return false;
            }
        } else {
            return false;
        }
        last = *ip;
        ip++;
    }
    if (value >= 0 && value <= 0xff) {
        if (3 == dots) {
            return true;
        }
    }
    return false;
}

inline long sIP2long(const char* ip)
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

// @brief get file content as string
// @example getFileAsCstring("test.txt") -> "file content"
inline std::string getFileAsCstring(const std::string& filename)
{
    std::string content{ };
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file.is_open()) {
        try {
            content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        } catch (const std::ios_base::failure& e) {
            Error("File read error: %s", e.what());
        }
        file.close();
    } else {
        Error("File open error: %s", strerror(errno));
    }
    return content;
}

// @brief get variable from url query string, or from a "k=v,k2=v2" config line
// @example getVariable("http://www.baidu.com?key=value&k2=v2", "key") -> "value"
//          getVariable("key=value,k2=v2\n", "key", ",") -> "value"
// delims: characters terminating the value; a trailing \r\n is always trimmed
inline std::string getVariable(const std::string& url, const std::string& key, const std::string& delims = "&")
{
    std::string val = { };
    size_t pos = url.find(key);
    if (pos != std::string::npos) {
        val = url.substr(pos, url.size());
        pos = val.find("=");
        size_t org = val.find_first_of(delims + "\r\n");
        if (org == std::string::npos) {
            val = val.substr(pos + 1, val.size() - pos - 1);
        } else {
            val = val.substr(pos + 1, org - pos - 1);
        }
    }
    return val;
}

// @brief split string by delimiter
// @example parseUri("xxx?/a/b/c") -> ["a", "b", "c"]
//          parseUri("xxx?/a/b/c?key=value") -> ["a", "b", "c"]
inline std::vector<std::string> parseUri(const std::string& uri)
{
    std::vector<std::string> vec{ };
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

// @brief split string by line
// @example linesToVec("a\nb\nc") -> ["a", "b", "c"]
//          linesToVec("a\r\nb\r\nc") -> ["a", "b", "c"]
//          linesToVec("a\r\nb\r\nc\r\n") -> ["a", "b", "c"]
inline std::vector<std::string> linesToVec(const std::string& str)
{
    std::vector<std::string> res{ };
    if (str.empty()) {
        return res;
    }
    size_t start = 0;
    while (true) {
        size_t pos = str.find('\n', start);
        if (pos != std::string::npos) {
            if (pos > 0 && str[pos - 1] == '\r') {
                std::string val = str.substr(start, pos - 1 - start);
                if (!val.empty())
                    res.push_back(val);
            } else {
                std::string val = str.substr(start, pos - start);
                if (!val.empty())
                    res.push_back(val);
            }
            start = pos + 1;
        } else {
            std::string val = str.substr(start);
            if (!val.empty())
                res.push_back(val);
            break;
        }
    }
    return res;
}

// @brief convert a string to a vector of strings, split by commas
// @example stringToVec("a,b,c", vec) -> vec = ["a", "b", "c"]
//          stringToVec("a,b,c,", vec) -> vec = ["a", "b", "c"]
inline void stringToVec(std::string str, std::vector<std::string>& vec)
{
    vec.clear();
    std::string tmp = "";
    while (!str.empty()) {
        std::string::size_type pos = str.find(",");
        if (pos == std::string::npos) {
            vec.push_back(str);
            break;
        }
        tmp = str.substr(0, pos);
        vec.push_back(tmp);
        str = str.substr(pos + 1);
    }
}
