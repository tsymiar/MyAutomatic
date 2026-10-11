#pragma once
/*
 * CTP 连接参数，账号与口令从配置文件读取。
 * 格式：每行 `key = value`，`#` 开头为注释；
 * 路径由环境变量 CTP_CONFIG 指定，缺省 ./ctp.conf。
 */
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

struct CtpConfig {
    std::string flow_path = "./";
    std::string front_addr = "tcp://180.168.146.187:10000";
    std::string broker_id = "88888888";
    std::string instrument_id = "i1409";
    std::string direction = "1";   // THOST_FTDC_D_Sell
    double limit_price = 2380;
    // 以下三项必须由配置文件提供，不设默认值
    std::string investor_id;
    std::string user_id;
    std::string password;
};

namespace ctpconf {

inline std::string trim(const std::string& s)
{
    const char* blank = " \t\r\n";
    const std::string::size_type begin = s.find_first_not_of(blank);
    if (begin == std::string::npos)
        return std::string();
    return s.substr(begin, s.find_last_not_of(blank) - begin + 1);
}

inline void copyString(char* dst, size_t size, const std::string& src)
{
    if (dst == NULL || size == 0)
        return;
    size_t n = src.size();
    if (n > size - 1)
        n = size - 1;
    memcpy(dst, src.c_str(), n);
    dst[n] = '\0';
}

inline const char* configPath()
{
    const char* path = getenv("CTP_CONFIG");
    return (path != NULL && *path != '\0') ? path : "./ctp.conf";
}

inline bool load(const char* path, CtpConfig& cfg)
{
    if (path == NULL || *path == '\0')
        return false;
    std::ifstream in(path);
    if (!in.is_open())
        return false;
    std::string line;
    while (std::getline(in, line)) {
        const std::string entry = trim(line);
        if (entry.empty() || entry[0] == '#')
            continue;
        const std::string::size_type eq = entry.find('=');
        if (eq == std::string::npos)
            continue;
        const std::string key = trim(entry.substr(0, eq));
        const std::string val = trim(entry.substr(eq + 1));
        if (key == "flow_path")
            cfg.flow_path = val;
        else if (key == "front_addr")
            cfg.front_addr = val;
        else if (key == "broker_id")
            cfg.broker_id = val;
        else if (key == "investor_id")
            cfg.investor_id = val;
        else if (key == "user_id")
            cfg.user_id = val;
        else if (key == "password")
            cfg.password = val;
        else if (key == "instrument_id")
            cfg.instrument_id = val;
        else if (key == "direction")
            cfg.direction = val;
        else if (key == "limit_price")
            cfg.limit_price = atof(val.c_str());
    }
    return true;
}

inline bool hasCredential(const CtpConfig& cfg)
{
    return !cfg.investor_id.empty()
        && !cfg.user_id.empty()
        && !cfg.password.empty();
}

}
