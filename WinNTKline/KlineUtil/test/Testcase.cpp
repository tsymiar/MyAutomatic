#include <ios>
#include <cstring>

#define EXCEPTION_MESSAGE_MAXLEN 256

template <class TestClass>
class TestCase
{
public:
    TestCase<TestClass>() { };
    TestCase<TestClass> getInstance();
    virtual ~TestCase<TestClass>();
private:
    TestCase<TestClass> instance;
    TestClass* testClass;
};

TestClass::testClass = new TestClass();
template <class TestClass>
TestCase<TestClass>
TestCase<TestClass>::getInstance()
{
    if (instance == nullptr)
        instance = new TestCase<TestClass>();
    return instance;
}

template <class TestClass>
TestCase<TestClass>::~TestCase()
{
    delete testClass;
    delete instance;
}

class Exception
{
private:
    char m_ExceptionMessage[EXCEPTION_MESSAGE_MAXLEN];
public:
    Exception(const char *msg)
    {
        strncpy_s(m_ExceptionMessage, msg, EXCEPTION_MESSAGE_MAXLEN);
    }
    inline char *GetMessage()
    {
        return m_ExceptionMessage;
    }
};
