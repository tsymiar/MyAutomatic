#ifdef _STRING_
//
#else
#ifndef __STRING_
#define __STRING_
#include <iostream>
#include <iomanip>
#include <string.h>

using namespace std;

class _string {
	friend ostream& operator<<(ostream&, _string&);
	friend istream& operator >> (istream&, _string&);
public:
	_string(const char* str = NULL);//赋值兼默认构造函数（char）
	_string(const _string& other);//赋值构造函数（String）
	_string& operator=(const _string& other);
	_string& operator+(const _string& other)const;
	bool operator==(const _string&);
	char& operator[](unsigned int);
	char* _strcpy(char* strDest, const char* strSrc);
	size_t size() {
		int len;
		for (len = 0; m_data[len]!= '\0'; len++)
			/*if (m_data[len] = '\0')
				break*/;
		if (len == (sizeof(m_data) / sizeof(char*) + 1))
			return len;
		else
			return strlen(m_data);
	}
	~_string(void) { delete[] m_data; }
private:
	char* m_data;
};

inline _string::_string(const char* str)//inline执行时直接语句替换
{
	if (!str)
		m_data = 0;
	else {
		m_data = new char[strlen(str) + 1];
		_strcpy(m_data, str);
	}
}
inline _string::_string(const _string& other)
{
	if (!other.m_data)//类的成员函数内可以访问同种对象的私有成员（同种类为友元关系的类）
		m_data = 0;
	else
	{
		m_data = new char[strlen(other.m_data) + 1];
		_strcpy(m_data, other.m_data);
	}
}

inline char* _string::_strcpy(char* strDest, const char* strSrc)
{
	assert((strDest != NULL) && (strSrc != NULL));
	char* strDestCopy = strDest;
	while ((*strDest++ = *strSrc++) != '\0')
		NULL;
	return strDestCopy;
}

inline _string & _string::operator=(const _string & other)
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

inline _string & _string::operator+(const _string & other) const
{
	_string newstring;
	if (!other.m_data)
		newstring = *this;
	else if (!m_data)
		newstring = other;
	else
	{
		newstring.m_data = new char[strlen(m_data) + strlen(other.m_data) + 1];
		strcpy(newstring.m_data, m_data);
		strcat(newstring.m_data, other.m_data);
	}
	return newstring;
}

inline bool _string::operator==(const _string& s)
{
	if (strlen(s.m_data) != strlen(m_data))
		return false;
	return strcmp(m_data, s.m_data) ? false : true;
}

inline char & _string::operator[](unsigned int e)
{
	if (e >= 0 && e <= strlen(m_data))
		return m_data[e];
}
inline ostream & operator<<(ostream& os, _string& str)
{
	os << str.m_data;
	return os;
}
inline istream & operator >> (istream &input, _string &s)
{
	char temp[255];//存储输入流
	input >> setw(255) >> temp;
	s = temp;//赋值
	return input;//return支持连续使用>>运算符
}
#endif
#endif
