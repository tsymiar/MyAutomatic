#include <ios>
#include <cstring>

#define EXCEPTION_MESSAGE_MAXLEN 256

template <class T>
class TestCase {
public:
    TestCase() : testClass(nullptr) {}
    // 持有裸指针资源，禁止拷贝（Rule of Three）
    TestCase(const TestCase&) = delete;
    TestCase& operator=(const TestCase&) = delete;
    // 函数内静态对象，防止递归构造
    static TestCase& getInstance()
    {
        static TestCase instance;
        return instance;
    }
    virtual ~TestCase()
    {
        delete testClass;
        testClass = nullptr;
    }
private:
    T* testClass;
};

class TestClass {};

class Exception {
private:
    char m_ExceptionMessage[EXCEPTION_MESSAGE_MAXLEN];
public:
    explicit Exception(const char* msg = nullptr) : m_ExceptionMessage{ NULL }
    {
        if (msg != nullptr) {
            strncpy_s(m_ExceptionMessage, msg, EXCEPTION_MESSAGE_MAXLEN);
        }
    }
    inline char* GetMessage()
    {
        return m_ExceptionMessage;
    }
};
