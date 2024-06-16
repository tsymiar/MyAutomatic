#pragma once

#if (defined __linux ) || (defined sprintf_s) || (!defined _WIN32)
#undef sprintf_s
#define sprintf_s snprintf
#endif // __linux

#define Concat(x,y) (x##y)
// #define ToChar(x) (#@x)
#define toString(x) (#x)

#ifdef _STRING_
#include <string>
//未使用STL的string类时
typedef std::string String_;
#define c_str_ c_str
#define size_ size
#define trim_ trim
#define equals_ equals
#define replace_ replace

inline unsigned char* avoid_str_err(unsigned char* str, unsigned int len)
{
    for (int i = 0; i < len; i++)
        switch (str[i]) {
        case 0xcc:/*烫 未初始化*/
        case 0xCD:/*heapk(new)*/
        case 0xDD://已收回的堆(delete)
        case 0xFD://隔离（栅栏字节）字节 下标越界
        case 0xAB://Memory allocated by LocalAlloc()
        case 0xBAADF00D://    Memory allocated by LocalAlloc() with LMEM_FIXED,
            //    but not yet written to.
        case 0xFEEEFEEE:/*  OS fill heap memory, which was marked for usage,\
                        but wasn't allocated by HeapAlloc() or LocalAlloc()\
                        Or that memory just has been freed by HeapFree().
                        */
            str[i] = '\0';
            break;
        default:break;
        }
    return str;
}
#else
#define _STRING__

#include <cassert>
#include <iomanip>
#include <istream>
#include <ostream>

using namespace std;

class String_ {
    friend ostream& operator<<(ostream&, const String_&);
    friend istream& operator >> (istream&, String_&);
public:
    String_(const char* str = nullptr);//赋值兼默认构造函数（char）
    String_(const String_& other);     //赋值构造函数（String_）
    ~String_(void) { if (m_data) delete[] m_data; }
    String_& operator=(const String_& other);
    String_/*&*/ operator+(const String_& other);
    String_ operator+=(const String_& other);
    bool operator==(const String_&);
    char& operator[](unsigned int) const;
    char* c_str_() const { return m_data; }
    size_t size_()
    {
        if (!m_data)
            return 0;
        int len;
        for (len = 0; m_data[len] != '\0'; len++) { ; }
        if (len == (sizeof(m_data) / sizeof(char*) + 1))
            return (size_t)len;
        else
            return strlen_(m_data);
    }
    String_ trim_();
    template<typename T> bool equals_(T object);
    int indexOf_(String_ str);
    String_ replace_(char old, char rplc);
    char charAt_(size_t z);
    String_ reverse_();
    String_ toLowerCase_();
    String_ toUpperCase_();
    static char* itoa_(int num, char* str, int radix);
    static int memcmp_(const void* ptr1, const void* ptr2, int size);
    static char* strcpy_(char* dest, const char* src, int len = 1024);
    static size_t strlen_(const char* str);
    static char* /*__cdecl*/strcat_(char* destination, const char* source);
    static int strcut_(unsigned char* str, char ch, char* str1, char* str2);
    static unsigned char* strsub_(unsigned char* src, int pos, int len);
    static char* reverse_(char* src, char* cst);
    static int char_count_(char* arr, char ch);
    static int char_count_array_(char** arr, char ch, int m = 1/*1st dim of arr*/);
    static char* str_ch_move_(char* w, char ch, int d, bool fore = false/*默认向后*/);
    static char* str_pos_move_(char* w, int m, int d, bool fore = false);
    static char* str_roll_move_(char* w, int m, bool fore = false);
private:
    char* m_data;
};
//inline执行时直接语句替换
inline String_::String_(const char* str)
{
    if (!str)
        m_data = 0;
    else {
        m_data = new char[strlen_(str) + 1];
        strcpy_(m_data, str);
    }
}

inline String_::String_(const String_& other)
{//类的成员函数内可以访问同种对象的私有成员（同种类为友元关系的类）
    if (!other.m_data)
        m_data = 0;
    else {
        m_data = new char[strlen_(other.m_data) + 1];
        strcpy_(m_data, other.m_data);
    }
}

inline String_& String_::operator=(const String_& other)
{
    if (this != &other) {
        delete[] m_data;
        if (!other.m_data)
            m_data = 0;
        else {
            m_data = new char[strlen_(other.m_data) + 1];
            strcpy_(m_data, other.m_data);
        }
    }
    return *this;
}

inline String_ /*&*/ String_::operator+(const String_& other)
{
    String_ new_string;
    if (!other.m_data)
        new_string = *this;
    else if (!m_data)
        new_string = other;
    else {
        new_string.m_data = new char[strlen_(m_data) + strlen_(other.m_data) + 1];
        new_string.strcpy_(new_string.m_data, m_data);
        new_string.strcat_(new_string.m_data, other.m_data);
    }//内联函数不该返回局部变量的引用
    return new_string;
}

inline String_ String_::operator+=(const String_& other)
{
    char* head = m_data;
    m_data = new char[this->size_() + strlen_(other.m_data) + 1];
    strcpy_(m_data, head);
    strcpy_(m_data + this->size_(), other.m_data);
    return *this;
}

inline bool String_::operator==(const String_& s)
{
    if (strlen_(s.m_data) != strlen_(m_data))
        return false;
    return memcmp_(m_data, s.m_data, (int)s.strlen_(m_data)) ? false : true;
}

inline char& String_::operator[](unsigned int e) const
{
    if (e <= strlen_(m_data))
        return m_data[e];
    else {
        static char s = '\0';
        return s;
    }
}

inline ostream& operator<<(ostream& os, const String_& str)
{
    os << str.m_data;
    return os;
}

inline istream& operator >> (istream& input, String_& s)
{
    char buf[255]; // 存储输入流
    input >> setw(255) >> buf;
    s = buf; // 赋值
    return input; // 支持连续使用>>运算符
}

inline String_ String_::trim_()
{
    unsigned int i = 0, j = 0, k = 0;
    size_t len = strlen_(m_data);
    for (; i < len; i++) {
        if (m_data[i] != ' ') {
            j = i;
            break;
        }
    }
    for (i = len - 1; i > 0; i--) {
        if (m_data[i] != ' ') {
            k = i;
            break;
        }
    }
    len = k - j + 1;
    char* str = new char[len];
    strcpy_(str, m_data + j, len);
    str[len] = '\0';
    return str;
}

template<typename T>
inline bool String_::equals_(T object)
{
    char* str = (char*)object;
    if (String_(str) == *this) {
        return true;
    }
    return false;
}

inline int String_::indexOf_(String_ str)
{
    size_t len = strlen_(m_data);
    for (unsigned int i = 0; i < len; i++) {
        if (memcmp_(&m_data[i], str.c_str_(), (int)str.size_()) == 0) {
            return i;
        }
    }
    return -1;
}

inline String_ String_::replace_(char old, char dst)
{
    size_t len = strlen_(m_data);
    char* str = new char[len];
    strcpy_(str, m_data, len);
    for (unsigned int i = 0; i < len; i++) {
        if (m_data[i] == old) {
            str[i] = dst;
        }
    }
    str[len] = '\0';
    return str;
}

inline char String_::charAt_(size_t z)
{
    if (z > strlen_(m_data)) {
        return 0;
    } else {
        return m_data[z];
    }
}

inline String_ String_::reverse_()
{
    size_t len = strlen_(m_data);
    char* str = new char[len + 1];
    strcpy_(str, m_data, len);
    for (unsigned int i = 0; i < len / 2; i++) {
        char t = str[i];
        str[i] = str[len - i - 1]; str[len - i - 1] = t;
    }
    str[len] = '\0';
    return str;
}

// 字符串逆序输出
inline char* String_::reverse_(char* src, char* cst)
{
    size_t len = strlen_(src);
    char* dest = (char*)malloc(len + 1);//为\0分配一个空间
    char* d = dest;
    char* s = &src[len - 1];//指向最后一个字符
    while (len-- != 0)
        *d++ = *s--;
    *d = 0;//尾部加\0
    strcpy_(cst, dest);
    assert(dest);
    free(dest);//释放空间
    return cst;
}

inline String_ String_::toLowerCase_()
{
    size_t len = strlen_(m_data);
    char* str = new char[len];
    strcpy_(str, m_data, len);
    for (unsigned int i = 0; i < len; i++) {
        char up = m_data[i];
        if (up >= 65 && up <= 90) {
            str[i] = m_data[i] + 32;
        }
    }
    str[len] = '\0';
    return str;
}

inline String_ String_::toUpperCase_()
{
    size_t len = strlen_(m_data);
    char* str = new char[len];
    strcpy_(str, m_data, len);
    for (unsigned int i = 0; i < len; i++) {
        char low = m_data[i];
        if (low >= 97 && low <= 122) {
            str[i] = m_data[i] - 32;
        }
    }
    str[len] = '\0';
    return str;
}

inline char* String_::itoa_(int num, char* str, int radix)
{
    if (num == 0) {
        str[0] = '0'; str[1] = '\0';
        return str;
    }
    char  string[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char* ptr = str;
    int i; int j;
    int value = num;
    if (num < 0) num = -num;
    while (num >= radix) {
        *ptr++ = string[num % radix];
        num /= radix;
    }
    if (num) {
        *ptr++ = string[num];
        *ptr = '\0';
    }
    int n = j = int(ptr - str - 1);
    for (i = 0; i < (ptr - str) / 2; i++) {
        int temp = str[i];
        str[i] = str[j];
        str[j--] = (char)temp;
    }
    if (value < 0) {
        for (j = n; j >= 0; --j) str[j + 1] = str[j];
        str[0] = '-';
    }
    str[n + 2] = '\0';
    return str;
}

inline int String_::memcmp_(const void* ptr1, const void* ptr2, int size)
{
    if (!size)
        return(0);
    while (--size && *(char*)ptr1 == *(char*)ptr2) {
        ptr1 = (char*)ptr1 + 1;
        ptr2 = (char*)ptr2 + 1;
    }
    return(*((unsigned char*)ptr1) - *((unsigned char*)ptr2));
}
//字符串拷贝
inline char* String_::strcpy_(char* dest, const char* src, int len)
{
    assert((dest != NULL) && (src != NULL));
    int available = len;
    char* strDestCopy = dest;
    while ((*dest++ = *src++) != '\0' && --available > 0)
        len = (int)NULL;
    return strDestCopy; //返回字符串以支持链式表达式
}
//计算字符个数（字符串长度）
inline size_t String_::strlen_(const char* str)
{
    if (str == NULL)
        throw "buffer is empty";
    const char* tmp = str;
    for (; *tmp != '\0'; ++tmp);
    return (size_t)(tmp - str);
}
//字符串拼接
inline char* /*__cdecl*/ String_::strcat_(char* destination, const char* source)
{
    if ((destination == NULL) || (source == NULL))
        throw "buffer is empty";
    char* cp = destination;
    while (*cp)
        cp++; /* find the end of destination */
    while (bool((*cp++ = *source++) != '\0')); /* Copy source to tail of destination */
    return(destination);
}
//截取字符串src内字符ch左右两边的子串，ch不保留。
inline int String_::strcut_(unsigned char* src, char ch, char* left, char* right)
{
    char s1[16];
    char s2[16];
    unsigned int i = 0;
    bool exist = false;
    for (i = 0; i < (unsigned int)strlen_((char*)src); i++)
        if (src[i] == ch) {
            exist = true;
            break;
        }
    if (!exist) {
        strcpy_(left, (const char*)src, (int)strlen_((const char*)src) + 1);
        right[0] = '\0';
        return -1;
    }
    sprintf_s(s1, 16, "%s", (char*)strsub_(src, 0, i));
    strcpy_(left, s1, (int)strlen_(s1) + 1);
    sprintf_s(s2, 16, "%s", (char*)strsub_(src, i + 1, (int)strlen_((char*)src) + 1 - i));
    strcpy_(right, s2, (int)strlen_(s2) + 1);
    return i;
}
//从第pos位开始截取src的len个字符。
inline unsigned char* String_::strsub_(unsigned char* src, int pos, int len)
{
    //定义一个字符指针，指向传递进来的ch地址
    unsigned char* pch = src;
    //通过calloc来分配一个len长度的字符数组，返回的是字符指针
    unsigned char* sub = (unsigned char*)calloc(sizeof(unsigned char), (size_t)(len + 1));
    int i;
    //只有在C99下for循环中才可以声明变量，这里写在外面，提高兼容性
    pch = pch + pos;
    //是pch指针指向pos位置
    for (i = 0; i < len; i++) {//循环遍历赋值数组
        sub[i] = *(pch++);
    }
    //加上字符串结束符
    sub[len] = '\0';
    //返回分配的字符数组地址
    return sub;
    //该地址指向的内存必须在函数外释放(free)
}

inline int String_::char_count_(char* arr, char ch)
{
    if (arr == NULL) return -1;
    int i = 0;
    char* str = arr;
    while (*(str = arr++) != '\0') {
        if (*str == '\0') {
            break;
        }
        if (*(str++) == ch) {
            i++;
        }
    }
    return i;
}
// 返回字符串数组str含有的字符ch数; Usage:
/*
    char l[][16] = { "acvhhj", "222", "ccc" };
    int m = 3;
    char *a[16] = { NULL };
    for(int i = 0; i < m; i++)
        a[i] = l[i]; // (char*)[const char* point]
    int s = char_count_array_(a, 'c', m);
*/
inline int String_::char_count_array_(char** arr, char ch, int m)
{
    if (arr == NULL) return -1;
    int i = 0;
    int num = 0;
    char* str = NULL;
    while ((str = *arr++) != NULL) {
        if (i >= m)
            break;
        while (*str != '\0') {
            if (*str++ == ch) {
                num++;
            }
        }
        i++;
    }
    return num;
}
/**
  * 单个字符的移动
  * w, 字符串
  * ch, 移动的字符
  * b, 移动的位数
  * fore, 移动的方向(默认后移)
  **/
inline char* String_::str_ch_move_(char* w, char ch, int d, bool fore)
{
    int i = 0;
    char* t = w;
    size_t len = strlen_(w);
    while (*t) {
        if (*t == ch)
            break;
        ++t;
        i++;
    }
    int m = i;
    char* r = new char[len + 1];
    strcpy_(r, w);
    if (fore) {
        (d > int(len) - m) ? (d = len - m) : 0;
        for (i = 0; i < d; i++)
            w[m + i] = *(r + m + (i + 1));
        w[m + d] = *(r + m);
    } else {
        (d > m) ? (d = m) : 0;
        for (i = 0; i < d; i++)
            w[m - i] = *(r + m - (i + 1));
        w[m - d] = *(r + m);
    }
    return w;
}
// 单个字符移动
// m: 字符位置
// d: 位数
// fore: 默认后移
inline char* String_::str_pos_move_(char* w, int m, int d, bool fore)
{
    int i = 0;
    m -= 1;
    size_t len = strlen_(w);
    char* s = new char[len + 1];
    strcpy_(s, w);
    if (fore) {
        (d > int(len) - m) ? (d = len - m) : 0;
        for (i = 0; i < d; i++)
            w[m + i] = s[m + (i + 1)];
        w[m + d] = s[m];
    } else {
        (d > m) ? (d = m) : 0;
        for (i = 0; i < d; i++)
            w[m - i] = s[m - (i + 1)];
        w[m - d] = s[m];
    }
    return w;
}
//字符串整体环状移动
inline char* String_::str_roll_move_(char* w, int m, bool fore)
{
    int i = 0;
    size_t len = strlen_(w);
    char* s = new char[len + 1];
    strcpy_(s, w);
    if (m > int(len))
        m = len;
    if (!fore) {
        while (w[m++] != '\0') {
            w[i++] = w[m - 1];
        }
        for (; i <= m; i++) {
            w[i] = *(s++);
        }
    } else {
        for (unsigned int t = 0; t < len - m; t++) {
            w[t + m] = *(s + t);
        }
        while (w[len - m + (i++)] != '\0') {
            w[i - 1] = *(s + len - m + i - 1);
        }
    }
    w[len] = '\0';
    return w;
}
#endif
