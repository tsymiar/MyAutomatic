#ifdef _STRING_
#include <string>
//未使用STL的string类时
typedef std::string String_;

inline unsigned char* fix_strerr(unsigned char* str)
{
    for (int i = 0; i < (int)strlen((const char*)str); i++)
        switch (str[i])
        {
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

#if (defined __linux ) || (defined sprintf_s)
#undef sprintf_s
#define sprintf_s sprintf
#endif // __linux

#define Concat(x,y) x##y
// #define ToChar(x) #@x
#define toString(x) #x

using namespace std;

class String_ {
    friend ostream& operator<<(ostream&, const String_&);
    friend istream& operator >> (istream&, String_&);
public:
    String_(const char* str = nullptr);//赋值兼默认构造函数（char）
    String_(const String_& other);  //赋值构造函数（String_）
    ~String_(void) { delete[] m_data; }
    String_& operator=(const String_& other);
    String_/*&*/ operator+(const String_& other);
    String_ operator+=(const String_& other);
    bool operator==(const String_&);
    char& operator[](unsigned int) const;
    char* c_str_() const {
        return m_data;
    };
    size_t size_() {
        int len;
        for (len = 0; m_data[len] != '\0'; len++)
            /*if (m_data[len] = '\0')
            break*/;
        if (len == (sizeof(m_data) / sizeof(char*) + 1))
            return len;
        else
            return strlen_(m_data);
    }
    String_ trim_();
    template<typename T> bool equals_(T object);
    int indexOf_(String_ str);
    String_ replace_(char old, char rplc);
    char charAt_(int z);
    String_ reverse_();
    String_ toLowerCase_();
    String_ toUpperCase_();
    static char* itoa_(int num, char *str, int radix);
    static int memcmp_(const void *buffer1, const void *buffer2, int count);
    static char* strcpy_(char* strDest, const char* strSrc, int LEN = 1024);
    static size_t strlen_(const char* str);
    static char* /*__cdecl*/strcat_(char * strDest, const char * strSrc);
    static int strcut_(unsigned char* str, char ch, char* str1, char* str2);
    static unsigned char* strsub_(unsigned char* ch, int pos, int len);
    static char* reverse_(char * src, char* cst);
    static int charcount_(char *arr, char ch);
    static int charcount2_(char **arr, char ch, int m = 1/*1st dim of arr*/);
    static char* charstrmove_(char* w, char ch, int d, bool fore = false/*默认向后*/);
    static char* charposmove_(char* w, int m, int d, bool fore = false);
    static char* strmove_(char* w, int m, bool fore = false);
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
    else
    {
        m_data = new char[strlen_(other.m_data) + 1];
        strcpy_(m_data, other.m_data);
    }
}

inline String_ & String_::operator=(const String_ & other)
{
    if (this != &other)
    {
        delete[] m_data;
        if (!other.m_data)
            m_data = 0;
        else
        {
            m_data = new char[strlen_(other.m_data) + 1];
            strcpy_(m_data, other.m_data);
        }
    }
    return *this;
}

inline String_ /*&*/ String_::operator+(const String_ & other)
{
    String_ newstring;
    if (!other.m_data)
        newstring = *this;
    else if (!m_data)
        newstring = other;
    else
    {
        newstring.m_data = new char[strlen_(m_data) + strlen_(other.m_data) + 1];
        newstring.strcpy_(newstring.m_data, m_data);
        newstring.strcat_(newstring.m_data, other.m_data);
    }//内联函数不该返回局部变量的引用
    return newstring;
}

inline String_ String_::operator+=(const String_ & other)
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
    return memcmp_(m_data, s.m_data, s.strlen_(m_data)) ? false : true;
}

inline char & String_::operator[](unsigned int e) const
{
    if (e >= 0 && e <= strlen_(m_data))
        return m_data[e];
}

inline ostream & operator<<(ostream& os, const String_& str)
{
    os << str.m_data;
    return os;
}

inline istream & operator >> (istream &input, String_ &s)
{
    char temp[255];//存储输入流
    input >> setw(255) >> temp;
    s = temp;//赋值
    return input;//支持连续使用>>运算符
}

inline String_ String_::trim_()
{
    int i = 0, j = 0, k = 0;
    int len = (int)strlen_(m_data);
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
    int len = (int)strlen_(m_data);
    for (int i = 0; i < len; i++) {
        if (memcmp_(&m_data[i], str.c_str_(), str.size_()) == 0) {
            return i;
        }
    }
    return -1;
}

inline String_ String_::replace_(char old, char rplc)
{
    int len = (int)strlen_(m_data);
    char* str = new char[len];
    strcpy_(str, m_data, len);
    for (int i = 0; i < len; i++) {
        if (m_data[i] == old) {
            str[i] = rplc;
        }
    }
    str[len] = '\0';
    return str;
}

inline char String_::charAt_(int z)
{
    if (z < 0 || z >(int)strlen_(m_data)) {
        return 0;
    } else {
        return m_data[z];
    }
}

inline String_ String_::reverse_()
{
    int len = (int)strlen_(m_data);
    char* str = new char[len + 1];
    strcpy_(str, m_data, len);
    for (int i = 0; i < len / 2; i++)
    {
        char t = str[i];
        str[i] = str[len - i - 1]; str[len - i - 1] = t;
    }
    str[len] = '\0';
    return str;
}

inline String_ String_::toLowerCase_()
{
    int len = (int)strlen_(m_data);
    char* str = new char[len];
    strcpy_(str, m_data, len);    
    for (int i = 0; i < len; i++) {
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
    int len = (int)strlen_(m_data);
    char* str = new char[len];
    strcpy_(str, m_data, len);
    for (int i = 0; i < len; i++) {
        char low = m_data[i];
        if (low >= 97 && low <= 122) {
            str[i] = m_data[i] - 32;
        }
    }
    str[len] = '\0';
    return str;
}

inline char* String_::itoa_(int num, char *str, int radix)
{
    if (num == 0)
    {
        str[0] = '0'; str[1] = '\0';
        return str;
    }
    char  string[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char* ptr = str;
    int i; int j;
    int value = num;
    if (num < 0) num = -num;
    while (num >= radix)
    {
        *ptr++ = string[num % radix];
        num /= radix;
    }
    if (num)
    {
        *ptr++ = string[num];
        *ptr = '\0';
    }
    int n = j = ptr - str - 1;
    for (i = 0; i < (ptr - str) / 2; i++)
    {
        int temp = str[i]; str[i] = str[j]; str[j--] = temp;
    }
    if (value < 0)
    {
        for (j = n; j >= 0; --j) str[j + 1] = str[j];
        str[0] = '-';
    }
    str[n + 2] = '\0';
    return str;
}

inline int String_::memcmp_(const void *buffer1, const void *buffer2, int count)
{
    if (!count)
        return(0);
    while (--count && *(char *)buffer1 == *(char *)buffer2)
    {
        buffer1 = (char *)buffer1 + 1;
        buffer2 = (char *)buffer2 + 1;
    }
    return(*((unsigned char *)buffer1) - *((unsigned char *)buffer2));
}
//字符串拷贝
inline char* String_::strcpy_(char* strDest, const char* strSrc, int LEN)
{
    assert((strDest != NULL) && (strSrc != NULL));
    size_t available = LEN;
    char* strDestCopy = strDest;
    while ((*strDest++ = *strSrc++) != '\0' && --available > 0)
        LEN = (int)NULL;
    return strDestCopy; //返回字符串以支持链式表达式
}
//计算字符个数（字符串长度）
inline size_t String_::strlen_(const char * str)
{
    if (str == NULL)
        throw "buffer is empty";
    const char* tmp = str;
    for (; *tmp != '\0'; ++tmp);
    return tmp - str;
}
//字符串拼接
inline char * /*__cdecl*/ String_::strcat_(char * strDest, const char * strSrc)
{
    if ((strDest == NULL) || (strSrc == NULL))
        throw "buffer is empty";
    char * cp = strDest;
    while (*cp)
        cp++; /* find end of strDest */
    while (bool((*cp++ = *strSrc++) != '\0')); /* Copy strSrc to end of strDest */
    return(strDest);
}
//截取字符串str内字符ch左右两边的子串，ch不保留。
inline int String_::strcut_(unsigned char* str, char ch, char* str1, char* str2)
{
    char s1[16];
    char s2[16];
    unsigned int i = 0;
    for (i = 0; i < (unsigned int)strlen_((char*)str); i++)
        if (str[i] == ch)
            break;
    sprintf_s(s1, "%s", (char*)strsub_(str, 0, i));
    strcpy_(str1, s1, strlen_(s1) + 1);
    sprintf_s(s2, "%s", (char*)strsub_(str, i + 1, (int)strlen_((char*)str) + 1 - i));
    strcpy_(str2, s2, strlen_(s2) + 1);
    return i;
}
//从第pos位开始截取ch的len个字符。
inline unsigned char* String_::strsub_(unsigned char* ch, int pos, int len)
{
    //定义一个字符指针，指向传递进来的ch地址
    unsigned char* pch = ch;
    //通过calloc来分配一个len长度的字符数组，返回的是字符指针
    unsigned char* subch = (unsigned char*)calloc(sizeof(unsigned char), len + 1);
    int i;
    //只有在C99下for循环中才可以声明变量，这里写在外面，提高兼容性  
    pch = pch + pos;
    //是pch指针指向pos位置
    for (i = 0; i < len; i++)
    {//循环遍历赋值数组
        subch[i] = *(pch++);
    }
    //加上字符串结束符
    subch[len] = '\0';
    //返回分配的字符数组地址
    return subch;
    //该地址指向的内存必须在函数外释放(free)
}
// 字符串逆序输出
inline char* String_::reverse_(char * src, char *cst)
{
    int len = (int)strlen_(src);
    char* dest = (char*)malloc(len + 1);//为\0分配一个空间
    char *d = dest;
    char* s = &src[len - 1];//指向最后一个字符
    while (len-- != 0)
        *d++ = *s--;
    *d = 0;//尾部加\0
    strcpy_(cst, dest);
    assert(dest);
    free(dest);//释放空间
    return cst;
}

inline int String_::charcount_(char *arr, char ch)
{
    int i = 0;
    char *str = arr;
    if (arr == NULL) {
        return -1;
    }
    while ((str = arr++) != '\0') {
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
    int s = charcount2_(a, 'c', m);
*/
inline int String_::charcount2_(char **arr, char ch, int m)
{
    int i = 0;
    int num = 0;
    char *str = NULL;
    if (arr == NULL)
        return -1;
    while ((str = *arr++) != NULL)
    {
        if (i >= m)
            break;
        while (*str != '\0')
        {
            if (*str++ == ch)
            {
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
inline char* String_::charstrmove_(char* w, char ch, int d, bool fore)
{
    int i = 0;
    char* t = w;
    int len = (int)strlen_(w);
    while (*t)
    {
        if (*t == ch)
            break;
        ++t;
        i++;
    }
    int m = i;
    char* r = new char[len + 1];
    strcpy_(r, w);
    if (fore)
    {
        (d > len - m) ? (d = len - m) : 0;
        for (i = 0; i < d; i++)
            w[m + i] = *(r + m + (i + 1));
        w[m + d] = *(r + m);
    } else
    {
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
inline char* String_::charposmove_(char* w, int m, int d, bool fore)
{
    int i = 0;
    m -= 1;
    int len = (int)strlen_(w);
    char* s = new char[len + 1];
    strcpy_(s, w);
    if (fore)
    {
        (d > len - m) ? (d = len - m) : 0;
        for (i = 0; i < d; i++)
            w[m + i] = s[m + (i + 1)];
        w[m + d] = s[m];
    } else
    {
        (d > m) ? (d = m) : 0;
        for (i = 0; i < d; i++)
            w[m - i] = s[m - (i + 1)];
        w[m - d] = s[m];
    }
    return w;
}
//字符串整体环状移动
inline char* String_::strmove_(char* w, int m, bool fore)
{
    int i = 0;
    int len = (int)strlen_(w);    
    char* s = new char[len + 1];
    strcpy_(s, w);
    if (m > len)
        m = len;
    if (!fore)
    {
        while (w[m++] != '\0')
        {
            w[i++] = w[m - 1];
        }
        for (; i <= m; i++)
        {
            w[i] = *(s++);
        }
    } else
    {
        for (int t = 0; t < len - m; t++)
        {
            w[t + m] = *(s + t);
        }
        while (w[len - m + (i++)] != '\0')
        {
            w[i - 1] = *(s + len - m + i - 1);
        }
    }
    w[len] = '\0';
    return w;
}
#endif
