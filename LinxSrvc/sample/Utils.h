#pragma once
#include <cstdio>
#include <libgen.h>
#include <string>
#include <vector>

#define Message(fmt, ...) fprintf(stdout, "\r[NOTICE](%s:%d)[%s]: " fmt "\n", basename(const_cast<char*>(__FILE__)),__LINE__,__FUNCTION__,##__VA_ARGS__)
#define Warning(fmt, ...) fprintf(stdout, "\r[WARN](%s:%d)[%s]: " fmt "\n", basename(const_cast<char*>(__FILE__)),__LINE__,__FUNCTION__,##__VA_ARGS__)
#define Error(fmt, ...) fprintf(stderr, "\r[ERROR](%s:%d)[%s]: " fmt "\n", basename(const_cast<char*>(__FILE__)),__LINE__,__FUNCTION__,##__VA_ARGS__)

bool isNum(const std::string& in);
bool ipIsValid(const char* ip);
long sIP2long(const char* ip);
std::string getFileAsCstring(const std::string& filename);
std::vector<std::string> parseUri(const std::string& uri);
std::string getVariable(const std::string& url, const std::string& key);
