#ifdef _STRING_
//未使用STL的string类时
#else
#ifndef __STRING_
#define __STRING_
#include <iostream>
#include <iomanip>
#include <string.h>
#include <assert.h>

using namespace std;

class _String {
	friend ostream& operator<<(ostream&, _String&);
	friend istream& operator >> (istream&, _String&);
public:
	_String(const char* str = NULL);//赋值兼默认构造函数（char）
	_String(const _String& other);//赋值构造函数（_String）
	_String& operator=(const _String& other);
	_String/*&*/ operator+(const _String& other)const;
	bool operator==(const _String&);
	char& operator[](unsigned int);
	char* _strcpy(char* strDest, const char* strSrc, int N = 1024);
	char* /*__cdecl*/_strcat(char * strDest, const char * strSrc);
	unsigned char* _strsub(unsigned char* ch, int pos, int len);
	size_t _strlen(const char* str);
	char* _strmove(char* w, int m, bool fore = false);
	char* _charmove(char* w, char c, int b, bool hind = false/*默认向左*/);
	char* _intmove(char* w, int m, int b, bool hind = false);
	char* _op_order(char * src, char* dst);
	char* _op_order(char * str);
	char* _c_str();
	size_t size() {
		int len;
		for (len = 0; m_data[len] != '\0'; len++)
			/*if (m_data[len] = '\0')
			break*/;
		if (len == (sizeof(m_data) / sizeof(char*) + 1))
			return len;
		else
			return strlen(m_data);
	}
	~_String(void) { delete[] m_data; }
private:
	char* m_data;
};

inline _String::_String(const char* str)//inline执行时直接语句替换
{
	if (!str)
		m_data = 0;
	else {
		m_data = new char[strlen(str) + 1];
		_strcpy(m_data, str);
	}
}

inline _String::_String(const _String& other)
{
	if (!other.m_data)//类的成员函数内可以访问同种对象的私有成员（同种类为友元关系的类）
		m_data = 0;
	else
	{
		m_data = new char[strlen(other.m_data) + 1];
		_strcpy(m_data, other.m_data);
	}
}
//字符串拷贝函数
inline char* _String::_strcpy(char* strDest, const char* strSrc, int N)
{
	assert((strDest != NULL) && (strSrc != NULL));
	size_t available = N;
	char* strDestCopy = strDest;
	while ((*strDest++ = *strSrc++) != '\0' && --available > 0)
		N = (int)NULL;
	return strDestCopy;//返回字符串以支持链式表达式
}
//字符串拼接
inline char * /*__cdecl*/ _String::_strcat(char * strDest, const char * strSrc)
{
	if ((strDest == NULL) || (strSrc == NULL))
		throw "buffer is empty";
	char * cp = strDest;
	while (*cp)
		cp++; /* find end of strDest */
	while (bool((*cp++ = *strSrc++) != false)); /* Copy strSrc to end of strDest */
	return(strDest);
}
//从第pos位开始截取len个字符。
inline unsigned char* _String::_strsub(unsigned char* ch, int pos, int len)
{
	unsigned char* pch = ch;
	//定义一个字符指针，指向传递进来的ch地址。  
	unsigned char* subch = (unsigned char*)calloc(sizeof(unsigned char), len + 1);
	//通过calloc来分配一个len长度的字符数组，返回的是字符指针。  
	int i;
	//只有在C99下for循环中才可以声明变量，这里写在外面，提高兼容性。  
	pch = pch + pos;
	//是pch指针指向pos位置。  
	for (i = 0; i<len; i++)
	{
		subch[i] = *(pch++);
		//循环遍历赋值数组。  
	}
	subch[len] = '\0';//加上字符串结束符。  
	return subch;       //返回分配的字符数组地址。  
}
//计算字符个数（字符串长度）
inline size_t _String::_strlen(const char * str)
{
	if (str == NULL)
		throw "buffer is empty";
	const char* tmp = str;
	for (; *tmp != '\0'; ++tmp);
	return tmp - str;
}
//字符串整体向左移动
inline char* _String::_strmove(char* w, int m, bool fore)
{
	int i = 0, len = strlen(w);
	char s = *w;
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
			w[i] = s++;
		}
	}
	else
	{
		for (int t = 0; t < len - m; t++)
		{
			w[t + m] = s + t;
		}
		while (w[len - m + (i++)] != '\0')
		{
			w[i - 1] = s + len - m + i - 1;
		}
	}
	w[len] = '\0';
	return w;
}
//单个字符的移动
/*字符串 w
*移动的字符 c
*移动的位数 b
*移动的方向 hind
*/
inline char* _String::_charmove(char* w, char c, int b, bool hind)
{
	int i = 0;
	char s = *w;
	char* t = w;
	while (*t)
	{
		if (*t == c)
			break;
		++t;
	}
	int m = c - s;
	if (hind)
	{
		(b > (int)_strlen(w) - m) ? (b = (int)_strlen(w) - m) : 0;
		for (i = 0; i < b; i++)
			w[m + i] = c + (i + 1);
		w[m + b] = c;
	}
	else
	{
		(b > m) ? (b = m) : 0;
		for (i = 0; i < b; i++)
			w[m - i] = c - (i + 1);
		w[m - b] = c;
	}
	return w;
}
//m:字符位置
inline char* _String::_intmove(char* w, int m, int b, bool hind)
{
	int i = 0;
	m -= 1;
	if (hind)
	{
		(b > (int)_strlen(w) - m) ? (b = (int)_strlen(w) - m) : 0;
		for (i = 0; i < b; i++)
			w[m + i] = *w + m + (i + 1);
		w[m + b] = *w + m;
	}
	else
	{
		(b > m) ? (b = m) : 0;
		for (i = 0; i < b; i++)
			w[m - i] = *w + m - (i + 1);
		w[m - b] = *w + m;
	}
	return w;
}
//字符串逆序输出
inline char* _String::_op_order(char * src, char *cst)
{
	int len = strlen(src);
	char* dest = (char*)malloc(len + 1);//为\0分配一个空间
	char *d = dest;
	char* s = &src[len - 1];//指向最后一个字符
	while (len-- != 0)
		*d++ = *s--;
	*d = 0;//尾部加\0
	_strcpy(cst, dest);
	assert(dest);
	free(dest);//释放空间
	return cst;
}

inline char* _String::_op_order(char * str)
{
	int len = strlen(str);
	char t;
	for (int i = 0; i < len / 2; i++)
	{
		t = str[i];
		str[i] = str[len - i - 1]; str[len - i - 1] = t;
	}
	return str;
}

inline char* _String::_c_str()
{
	return m_data;
}

inline _String & _String::operator=(const _String & other)
{
	if (this != &other)
	{
		delete[] m_data;
		if (!other.m_data)
			m_data = 0;
		else
		{
			m_data = new char[strlen(other.m_data) + 1];
			_strcpy(m_data, other.m_data);
		}
	}
	return *this;
}

inline _String /*&*/ _String::operator+(const _String & other) const
{
	_String newstring;
	if (!other.m_data)
		newstring = *this;
	else if (!m_data)
		newstring = other;
	else
	{
		newstring.m_data = new char[strlen(m_data) + strlen(other.m_data) + 1];
		newstring._strcpy(newstring.m_data, m_data);
		newstring._strcat(newstring.m_data, other.m_data);
	}
	return newstring;//内联函数不该返回局部变量的引用
}

inline bool _String::operator==(const _String& s)
{
	if (strlen(s.m_data) != strlen(m_data))
		return false;
	return strcmp(m_data, s.m_data) ? false : true;
}

inline char & _String::operator[](unsigned int e)
{
	if (e >= 0 && e <= strlen(m_data))
		return m_data[e];
}
inline ostream & operator<<(ostream& os, _String& str)
{
	os << str.m_data;
	return os;
}
inline istream & operator >> (istream &input, _String &s)
{
	char temp[255];//存储输入流
	input >> setw(255) >> temp;
	s = temp;//赋值
	return input;//return支持连续使用>>运算符
}
#endif
#endif
