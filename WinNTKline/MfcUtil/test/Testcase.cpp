#include <ios>
#include <cstring>

#define EXCEPTION_MESSAGE_MAXLEN 256

template <class clazz>
class TestCase
{
public:
    TestCase<clazz>() { klas = new clazz(); };
    TestCase<clazz> getInstance();
    virtual ~TestCase<clazz>();
private:
    TestCase<clazz> instance;
    clazz* klas;
};

template <class clazz>
TestCase<clazz>
TestCase<clazz>::getInstance()
{
    if (instance == nullptr)
        instance = new TestCase<clazz>();
    return instance;
}

template <class clazz>
TestCase<clazz>::~TestCase()
{
    delete[] klas, instance;
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
