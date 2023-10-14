#include "test.h"
#include "String_-inl.h"
#include <fstream>
#include <string.h>

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
    if (fbuf.open("test.txt", std::ios::out) != nullptr) {
        std::ostream os(&fbuf);
        os << s2;
    }
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
    EXPECT_EQ(static_cast<int>(s0.size_()), 0);
    String_ s1("s");
    EXPECT_EQ(static_cast<int>(s1.size_()), 1);
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

TEST(String_, reverse_c_c)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    char s2[16];
    EXPECT_STREQ(String_::reverse_(s1, s2), s2);
    EXPECT_STREQ(s2, "!TSET a si sihT");
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
    EXPECT_STREQ(String_::itoa_(0, s1, 1), "0");
    EXPECT_STREQ(s1, "0");
    EXPECT_STREQ(String_::itoa_(77, s1, 10), "77");
    EXPECT_STREQ(s1, "77");
    EXPECT_STREQ(String_::itoa_(36, s1, 16), "24");
    EXPECT_STREQ(s1, "24");
    EXPECT_STREQ(String_::itoa_(-5, s1, 10), "-5");
    EXPECT_STREQ(s1, "-5");
}

TEST(String_, strcut_)
{
    unsigned char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    char st1[16];
    char st2[16];
    String_::strcut_(s1, 'a', st1, st2);
    EXPECT_STREQ(st1, "This is ");
    EXPECT_STREQ(st2, " TEST!");
    memset(st1, 0, 16);
    memset(st2, 0, 16);
    String_::strcut_(s1, 'x', st1, st2);
    EXPECT_STREQ(st1, "This is a TEST!");
    EXPECT_STREQ(st2, "");
}

TEST(String_, char_count_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_EQ(String_::char_count_(s1, 'T'), 3);
    EXPECT_EQ(String_::char_count_(s1, 'i'), 2);
    EXPECT_EQ(String_::char_count_(s1, 'h'), 1);
    EXPECT_EQ(String_::char_count_(s1, 'x'), 0);
    EXPECT_EQ(String_::char_count_(nullptr, 'x'), -1);
}

TEST(String_, char_count_array_)
{
    char l[][16] = { "acvhhj", "222", "ccc" };
    int m = 3;
    char* a[16] = { NULL };
    for (int i = 0; i < m; i++)
        a[i] = l[i]; // (char*)[const char* point]
    int s = String_::char_count_array_(a, 'c', m);
    EXPECT_EQ(s, 4);
    s = String_::char_count_array_(nullptr, 'x', m);
    EXPECT_EQ(s, -1);
}

TEST(String_, str_ch_move_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_STREQ(String_::str_ch_move_(s1, 'T', 3, true), "hisT is a TEST!");
    EXPECT_STREQ(String_::str_ch_move_(s1, 'a', 3, false), "hisT ais  TEST!");
}

TEST(String_, str_pos_move_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_STREQ(String_::str_pos_move_(s1, 9, 3), "This ais  TEST!");
    EXPECT_STREQ(String_::str_pos_move_(s1, 4, 5, true), "Thi ais s TEST!");
}

TEST(String_, str_roll_move_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_STREQ(String_::str_roll_move_(s1, 3), "s is a TEST!Thi");
    EXPECT_STREQ(String_::str_roll_move_(s1, 3, true), "This is a TEST!");
    EXPECT_STREQ(String_::str_roll_move_(s1, 32), "This is a TEST!");
}
