#include "test.h"
#include "Utils.h"

#undef Message
#undef Warning
#undef Error

TEST(Utils, isNum)
{
    std::string num = "xyz";
    EXPECT_FALSE(isNum(num));
    num = "123";
    EXPECT_TRUE(isNum(num));
    num = "0.5";
    EXPECT_TRUE(isNum(num));
}

TEST(Utils, ipIsValid)
{
    const char* ip = "xyz";
    EXPECT_FALSE(ipIsValid(ip));
    ip = "";
    EXPECT_FALSE(ipIsValid(ip));
    ip = "01";
    EXPECT_FALSE(ipIsValid(ip));
    ip = "....";
    EXPECT_FALSE(ipIsValid(ip));
    ip = "0...";
    EXPECT_FALSE(ipIsValid(ip));
    ip = "0.1.2.3";
    EXPECT_FALSE(ipIsValid(ip));
    ip = "127.0.0.0";
    EXPECT_FALSE(ipIsValid(ip));
    ip = "127.0.0.1";
    EXPECT_TRUE(ipIsValid(ip));
    ip = "192.168.0.1";
    EXPECT_TRUE(ipIsValid(ip));
    ip = "123.123.123.123";
    EXPECT_TRUE(ipIsValid(ip));
}

TEST(Utils, sIP2long)
{
    const char* sip = "127.0.0.1";
    EXPECT_EQ(sIP2long(sip), 2130706433);
    sip = "192.168.0.1";
    EXPECT_NE(sIP2long(sip), 2130706433);
    sip = "123.123.123.123";
    EXPECT_NE(sIP2long(sip), 2130706433);
}

TEST(Utils, parseUri)
{
    std::vector<std::string> vals;
    std::string uri = "xyz";
    EXPECT_EQ(parseUri(uri), vals);
    uri = "x/y/z";
    vals.push_back("x");
    vals.push_back("y");
    vals.push_back("z");
    EXPECT_EQ(parseUri(uri), vals);
}

TEST(Utils, getVariable)
{
    std::string url = "xyz";
    std::string key = "0";
    EXPECT_EQ(getVariable(url, key), "");
    url = "i=t";
    key = "i";
    EXPECT_EQ(getVariable(url, key), "t");
    url = "i=t&c=v&";
    key = "c";
    EXPECT_EQ(getVariable(url, key), "v");
}

TEST(Utils, getFileAsCstring)
{
    std::string filename = "";
    EXPECT_EQ(getFileAsCstring(filename), "");
    filename = "./test.txt";
    EXPECT_EQ(getFileAsCstring(filename), "aaa");
}
