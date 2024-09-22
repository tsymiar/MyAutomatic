#pragma once

#include <cmath>
#include <cfloat>
#include <cassert>
#ifdef _MSC_VER
#include <atlstr.h>
#endif

class Converts {

public:
    Converts() { }

private:
    char* m_ptr;
public:
#ifdef _MSC_VER
    char* AllocBuffer(CString msg)
    {
#ifdef _UNICODE
        long len = WideCharToMultiByte(CP_ACP, 0, msg, -1, NULL, 0, NULL, NULL);
        m_ptr = new char[m_len + 1];
        memset(m_ptr, 0, m_len + 1);
        WideCharToMultiByte(CP_ACP, 0, msg, -1, m_ptr, m_len + 1, NULL, NULL);
#else
        m_ptr = new char[msg.GetAllocLength() + 1];
        sprintf_s(m_ptr, 1024, _T("%s"), (LPSTR)(LPCTSTR)msg);
#endif // _UNICODE
        char* p = m_ptr;
        assert(m_ptr != NULL);
        return p;
    }
#endif
    virtual ~Converts()
    {
        if (m_ptr != nullptr)
        {
            delete (char*)m_ptr;
            m_ptr = nullptr;
        }
    }
};
