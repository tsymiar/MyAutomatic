#include <ios>
#include <cstring>

#define EXCEPTION_MESSAGE_MAXLEN 256

template <class Class>
class MyTest
{
public:
	MyTest<Class>();
	virtual ~MyTest<Class>();
	Class* Obj;
};

template <class Class>
MyTest<Class>::MyTest()
{
	Obj=new Class();
}

template <class Class>
MyTest<Class>::~MyTest()
{
	delete Obj;
}

class Exception
{
private:
	char m_ExceptionMessage[EXCEPTION_MESSAGE_MAXLEN];
public:
	Exception(char *msg)
	{
		strncpy_s(m_ExceptionMessage, msg, EXCEPTION_MESSAGE_MAXLEN);
	}
	inline char *GetMessage()
	{
		return m_ExceptionMessage;
	}
};
