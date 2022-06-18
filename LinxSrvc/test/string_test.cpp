#include "test.h"
#include "String_-inl.h"
#include <fstream>

TEST(String_, operator)
{
    String_ s0(nullptr);
    String_ s0s = "s0s";
    // s0s += s0;
    s0 = s0s;
    s0s += s0;
    String_ s1 = s0s + s0;
    EXPECT_STREQ(s1.c_str_(), "s0ss0ss0s");
    EXPECT_EQ(s1[1], '0');
    EXPECT_EQ(s1[100], '\0');
    String_ s2 = "aaa";
    std::filebuf fbuf;
    fbuf.open("test.txt", std::ios::out);
    std::ostream os(&fbuf);
    os << s2;
    fbuf.close();
    if (fbuf.open("test.txt", std::ios_base::in) != nullptr) {
        std::istream is(&fbuf);
        String_ s3;
        is >> s3;
        EXPECT_STREQ(s3.c_str_(), "aaa");
    }
    fbuf.close();
}

TEST(String_, c_str_)
{
    String_ s0(nullptr);
    EXPECT_STREQ(s0.c_str_(), nullptr);
    String_ s0s = s0;
    EXPECT_STREQ(s0s.c_str_(), nullptr);
    String_ s1("s");
    EXPECT_STREQ(s1.c_str_(), "s");
    String_ s1s = s1;
    EXPECT_STREQ(s1s.c_str_(), "s");
}

TEST(String_, size_)
{
    String_ s0(nullptr);
    EXPECT_EQ(s0.size_(), 0);
    String_ s1("s");
    EXPECT_EQ(s1.size_(), 1);
}

TEST(String_, trim_)
{
    String_ s1("s");
    EXPECT_TRUE(s1.trim_() == s1);
    String_ s2(" s ");
    EXPECT_STREQ(s2.trim_().c_str_(), "s");
}

TEST(String_, equals_)
{
    String_ s1("s");
    EXPECT_FALSE(s1.equals_("S"));
    EXPECT_TRUE(s1.equals_("s"));
}

TEST(String_, indexOf_)
{
    String_ s1("This is a TEST!");
    EXPECT_EQ(s1.indexOf_("s"), 3);
    EXPECT_EQ(s1.indexOf_("x"), -1);
}

TEST(String_, replace_)
{
    String_ s1("This is a TEST!");
    EXPECT_TRUE(s1.replace_('s', 'm') == String_("Thim im a TEST!"));
}

TEST(String_, charAt_)
{
    String_ s1("This is a TEST!");
    EXPECT_EQ(s1.charAt_(3), 's');
    EXPECT_EQ(s1.charAt_(-1), 0);
    EXPECT_EQ(s1.charAt_(100), 0);
}

TEST(String_, reverse_)
{
    String_ s1("This is a TEST!");
    EXPECT_TRUE(s1.reverse_() == String_("!TSET a si sihT"));
}

TEST(String_, toLowerCase_)
{
    String_ s1("This is a TEST!");
    EXPECT_TRUE(s1.toLowerCase_() == String_("this is a test!"));
}

TEST(String_, toUpperCase_)
{
    String_ s1("This is a TEST!");
    EXPECT_TRUE(s1.toUpperCase_() == String_("THIS IS A TEST!"));
}

TEST(String_, itoa_)
{
    char s1[8];
    String_ s0;
    EXPECT_STREQ(s0.itoa_(0, s1, 1), "0");
    EXPECT_STREQ(s0.itoa_(77, s1, 10), "77");
    EXPECT_STREQ(s0.itoa_(36, s1, 16), "24");
    EXPECT_STREQ(s0.itoa_(-5, s1, 10), "-5");
}
