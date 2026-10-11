#pragma once

#include <cmath>
#include <cfloat>
#include <cassert>
#include <new>
#ifdef _MSC_VER
#include <atlstr.h>
#endif

/*
 * 返回 new char[]，所有权转移给消息接收方，用完后 delete[]。
 * PostMessage 是异步的，缓冲区不能由本对象持有。
 */
class Converts {

public:
    Converts() { }

public:
    // 禁止拷贝
    Converts(const Converts&) = delete;
    Converts& operator=(const Converts&) = delete;

#ifdef _MSC_VER
    char* AllocBuffer(CString msg)
    {
        char* buf = nullptr;
#ifdef _UNICODE
        // WideCharToMultiByte 返回值已包含结尾 '\0'
        int need = WideCharToMultiByte(CP_ACP, 0, msg, -1, NULL, 0, NULL, NULL);
        if (need <= 0)
            return nullptr;
        buf = new (std::nothrow) char[need];
        if (buf == nullptr)
            return nullptr;
        buf[0] = '\0';
        if (WideCharToMultiByte(CP_ACP, 0, msg, -1, buf, need, NULL, NULL) <= 0)
            buf[0] = '\0';
#else
        int need = msg.GetLength() + 1;
        buf = new (std::nothrow) char[need];
        if (buf == nullptr)
            return nullptr;
        buf[0] = '\0';
        // 按实际容量拷贝
        _tcsncpy_s(buf, need, (LPCTSTR)msg, _TRUNCATE);
#endif // _UNICODE
        return buf;
    }
#endif

    virtual ~Converts() { }
};
