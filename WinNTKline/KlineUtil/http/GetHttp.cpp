#include "GetHttp.h"
#include <wininet.h>

using namespace std;

CString HttpGet(string& sRequest, string sPort)
{
    HINTERNET hSession = NULL;
    HINTERNET hConnection = NULL;
    HINTERNET hRequest = NULL;
    LPCTSTR sAcceptTypes = NULL;
    string sHeaders;
    string sResponse = "";
    string sLog;
    int nRet = -1;
    int nCount = 0;
    char szRet[512];
    char szBuf[8192];
    DWORD dwRetLen = 0;
    DWORD dwReadLen = 0;

    int t = 0;
    string lpszName = sRequest;
    lpszName.erase(0, lpszName.find("://") + 3);
    string sHostName = lpszName;
    lpszName.erase(0, t = lpszName.find("/") + 1);
    sHostName.erase(t - 1, sHostName.length() - t + 1);

    DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION;
    hSession = ::InternetOpen("RookIE/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    if (hSession == NULL)
    {
        goto LERROR;
    }

    dwFlags |= INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    //创建http连接
    hConnection = ::InternetConnect(hSession, sHostName.c_str(), atoi(sPort.c_str()), NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

    if (hConnection == NULL)
    {
        goto LERROR;
    }

    //开启POST请求
    sAcceptTypes = "text/*";
    hRequest = ::HttpOpenRequest(hConnection, "POST", lpszName.c_str(), HTTP_VERSION, NULL, &sAcceptTypes, dwFlags, 0);

    if (hRequest == NULL)
    {
        goto LERROR;
    }
    //添加请求报文头
    sHeaders = "Accept: \r\nContent-Type:text/xml;charset=utf-8";
    nRet = ::HttpAddRequestHeaders(hRequest, sHeaders.c_str(), -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);

    if (nRet == 0)
    {
        goto LERROR;
    }
    //发送请求
    int nLen = sRequest.length();
    nRet = ::HttpSendRequest(hRequest, NULL, -1, (void*)sRequest.c_str(), nLen);

    int nErrorCount = 0;
    while (0 == nRet && nErrorCount < 3)
    {
        nErrorCount++;
        DWORD dwError = GetLastError();
        if (dwError == ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED)
        {
            LPVOID  dwCert = NULL;
            InternetErrorDlg((HWND)GetDesktopWindow(),
                hRequest,
                ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED,
                FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
                FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
                &dwCert);
            InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_SELECT_CLIENT_CERT, dwCert, sizeof(dwCert));
        }
        else if (dwError == ERROR_INTERNET_INVALID_CA)
        {
            DWORD  dwBuffLen = sizeof(dwFlags);
            InternetQueryOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);
            dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
            InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
        }
        else
        {
            break;
        }
    }
    if (0 == nRet)
    {
        nRet = ::HttpSendRequest(hRequest, NULL, -1, (void*)sRequest.c_str(), nLen);
    }
    if (nRet == 0)
    {
        goto LERROR;
    }

    //查询返回信息
    dwRetLen = 1024;
    memset(szRet, 0x0, sizeof(szRet));
    nRet = ::HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, szRet, &dwRetLen, NULL);

    if (nRet == 0)
    {
        goto LERROR;
    }
    nRet = atoi(szRet);

    if (HTTP_STATUS_PROXY_AUTH_REQ == nRet)
    {
        InternetSetOption(hRequest, INTERNET_OPTION_PROXY_USERNAME, "User-001", strlen("User-001"));
        InternetSetOption(hRequest, INTERNET_OPTION_PROXY_PASSWORD, "111111", strlen("111111"));
        nRet = ::HttpSendRequest(hRequest, NULL, -1, (void*)sRequest.c_str(), nLen);

        if (nRet == 0)
        {
            goto LERROR;
        }

        dwRetLen = 1024;
        memset(szRet, 0x0, sizeof(szRet));

        nRet = ::HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE, szRet, &dwRetLen, NULL);
        if (nRet == 0)
        {
            goto LERROR;
        }
        nRet = atoi(szRet);
    }
    if (nRet != HTTP_STATUS_OK)
    {
        goto LERROR;
    }
    sResponse.clear();
    //读取文件
    do
    {
        memset(szBuf, 0x0, sizeof(szBuf));
        ::InternetReadFile(hRequest, szBuf, sizeof(szBuf), &dwReadLen);
        sResponse += szBuf;
        nCount++;
    } while (dwReadLen != 0);

LERROR:	//错误处理
    ::InternetCloseHandle(hRequest);
    ::InternetCloseHandle(hConnection);
    ::InternetCloseHandle(hSession);

    return sResponse.c_str();
}
