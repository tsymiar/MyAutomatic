#include "test.h"
#include "String_-inl.h"
#include <fstream>
#include <string.h>

TEST(String_s, operator)
{
    String_s s0(nullptr);
    String_s s0s = "s0s";
    // s0s += s0;
    s0 = s0s;
    s0s += s0;
    String_s s1 = s0s + s0;
    EXPECT_STREQ(s1.c_str_(), "s0ss0ss0s");
    EXPECT_EQ(s1[1], '0');
    EXPECT_EQ(s1[100], '\0');
    String_s s2 = "aaa";
    std::filebuf fbuf;
    if (fbuf.open("test.txt", std::ios::out) != nullptr) {
        std::ostream os(&fbuf);
        os << s2;
    }
    fbuf.close();
    if (fbuf.open("test.txt", std::ios_base::in) != nullptr) {
        std::istream is(&fbuf);
        String_s s3;
        is >> s3;
        EXPECT_STREQ(s3.c_str_(), "aaa");
    }
    fbuf.close();
}

TEST(String_s, c_str_)
{
    String_s s0(nullptr);
    EXPECT_STREQ(s0.c_str_(), nullptr);
    String_s s0s = s0;
    EXPECT_STREQ(s0s.c_str_(), nullptr);
    String_s s1("s");
    EXPECT_STREQ(s1.c_str_(), "s");
    String_s s1s = s1;
    EXPECT_STREQ(s1s.c_str_(), "s");
}

TEST(String_s, size_)
{
    String_s s0(nullptr);
    EXPECT_EQ(static_cast<int>(s0.size_()), 0);
    String_s s1("s");
    EXPECT_EQ(static_cast<int>(s1.size_()), 1);
}

TEST(String_s, trim_)
{
    String_s s1("s");
    EXPECT_TRUE(s1.trim_() == s1);
    String_s s2(" s ");
    EXPECT_STREQ(s2.trim_().c_str_(), "s");
}

TEST(String_s, equals_)
{
    String_s s1("s");
    EXPECT_FALSE(s1.equals_("S"));
    EXPECT_TRUE(s1.equals_("s"));
}

TEST(String_s, indexOf_)
{
    String_s s1("This is a TEST!");
    EXPECT_EQ(s1.indexOf_("s"), 3);
    EXPECT_EQ(s1.indexOf_("x"), -1);
}

TEST(String_s, replace_)
{
    String_s s1("This is a TEST!");
    EXPECT_TRUE(s1.replace_('s', 'm') == String_s("Thim im a TEST!"));
}

TEST(String_s, charAt_)
{
    String_s s1("This is a TEST!");
    EXPECT_EQ(s1.charAt_(3), 's');
    EXPECT_EQ(s1.charAt_(-1), 0);
    EXPECT_EQ(s1.charAt_(100), 0);
}

TEST(String_s, reverse_)
{
    String_s s1("This is a TEST!");
    EXPECT_TRUE(s1.reverse_() == String_s("!TSET a si sihT"));
}

TEST(String_s, reverse_c_c)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    char s2[16];
    EXPECT_STREQ(String_s::reverse_(s1, s2), s2);
    EXPECT_STREQ(s2, "!TSET a si sihT");
}

TEST(String_s, toLowerCase_)
{
    String_s s1("This is a TEST!");
    EXPECT_TRUE(s1.toLowerCase_() == String_s("this is a test!"));
}

TEST(String_s, toUpperCase_)
{
    String_s s1("This is a TEST!");
    EXPECT_TRUE(s1.toUpperCase_() == String_s("THIS IS A TEST!"));
}

TEST(String_s, itoa_)
{
    char s1[8];
    EXPECT_STREQ(String_s::itoa_(0, s1, 1), "0");
    EXPECT_STREQ(s1, "0");
    EXPECT_STREQ(String_s::itoa_(77, s1, 10), "77");
    EXPECT_STREQ(s1, "77");
    EXPECT_STREQ(String_s::itoa_(36, s1, 16), "24");
    EXPECT_STREQ(s1, "24");
    EXPECT_STREQ(String_s::itoa_(-5, s1, 10), "-5");
    EXPECT_STREQ(s1, "-5");
}

TEST(String_s, strcut_)
{
    unsigned char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    char st1[16];
    char st2[16];
    String_s::strcut_(s1, 'a', st1, st2);
    EXPECT_STREQ(st1, "This is ");
    EXPECT_STREQ(st2, " TEST!");
    memset(st1, 0, 16);
    memset(st2, 0, 16);
    String_s::strcut_(s1, 'x', st1, st2);
    EXPECT_STREQ(st1, "This is a TEST!");
    EXPECT_STREQ(st2, "");
}

TEST(String_s, char_count_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_EQ(String_s::char_count_(s1, 'T'), 3);
    EXPECT_EQ(String_s::char_count_(s1, 'i'), 2);
    EXPECT_EQ(String_s::char_count_(s1, 'h'), 1);
    EXPECT_EQ(String_s::char_count_(s1, 'x'), 0);
    EXPECT_EQ(String_s::char_count_(nullptr, 'x'), -1);
}

TEST(String_s, char_count_array_)
{
    char l[][16] = { "acvhhj", "222", "ccc" };
    int m = 3;
    char* a[16] = { NULL };
    for (int i = 0; i < m; i++)
        a[i] = l[i]; // (char*)[const char* point]
    int s = String_s::char_count_array_(a, 'c', m);
    EXPECT_EQ(s, 4);
    s = String_s::char_count_array_(nullptr, 'x', m);
    EXPECT_EQ(s, -1);
}

TEST(String_s, str_ch_move_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_STREQ(String_s::str_ch_move_(s1, 'T', 3, true), "hisT is a TEST!");
    EXPECT_STREQ(String_s::str_ch_move_(s1, 'a', 3, false), "hisT ais  TEST!");
}

TEST(String_s, str_pos_move_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_STREQ(String_s::str_pos_move_(s1, 9, 3), "This ais  TEST!");
    EXPECT_STREQ(String_s::str_pos_move_(s1, 4, 5, true), "Thi ais s TEST!");
}

TEST(String_s, str_roll_move_)
{
    char s1[16];
    memcpy(s1, "This is a TEST!", 16);
    EXPECT_STREQ(String_s::str_roll_move_(s1, 3), "s is a TEST!Thi");
    EXPECT_STREQ(String_s::str_roll_move_(s1, 3, true), "s a TEST!This i");
    EXPECT_STREQ(String_s::str_roll_move_(s1, 32), "s a TEST!This i");
}
