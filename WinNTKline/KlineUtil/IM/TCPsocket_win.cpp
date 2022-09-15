#pragma once
#include <stdio.h>
#include <iostream>
#include <process.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>
#include <conio.h>
#include <errno.h>
#define random(x) (rand() % x * 0.001f)
#pragma comment( lib, "ws2_32.lib" )
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma warning (disable:4996)
#define addr_out(ip) inet_ntoa(ip)
#ifndef EWOULDBLOCK
#define EWOULDBLOCK (10035L)
#endif

using namespace std;

///////////////////////////////////////////////
unsigned int __stdcall SocketThread(void* lp)
{
    int port = 6001;
    if (lp != nullptr) {
        int arg = (*(int*)lp);
        if (arg != 0)
            port = arg;
    }
    srand((int)time(0));
    cout << "启动TCP服务: " << port << endl;
    // 用于WSAstartup填充
    WSADATA wsa;
    // WSAstartup初始化WINDOWS SOCKET API
    int err = WSAStartup(0x101, &wsa);

    if (err != 0) {
        cout << "初始化套接字失败！" << endl;
        return -1;
    }
    // 重置字节流套接口
    if (LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1) {
        WSACleanup();
        return 0;
    }
    // 设置服务器端套接字 基于TCP
    SOCKET serSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serSock == INVALID_SOCKET) {
        cout << "无效的套接字！" << endl;
        return -2;
    }
    // 描述本地端口信息
    sockaddr_in local;
    // 接受所有连接
    local.sin_addr.S_un.S_addr = INADDR_ANY;
    local.sin_family = AF_INET;
    // 本地服务器端口号
    local.sin_port = htons((u_short)port);
    int optval = 1;
    // 检测对方主机是否崩溃，避免（服务器）永远阻塞于TCP连接的输入。
    setsockopt(serSock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
    cout << "绑定..." << endl;//绑定
    if (bind(serSock, (sockaddr*)&local, sizeof(local)) != 0) {
        closesocket(serSock);
        cout << "绑定端口失败，端口被占用！" << endl;
        return -3;
    }
    // 监听
    cout << "监听..." << endl;
    if (listen(serSock, 5) != 0) {
        closesocket(serSock);
        cout << "监听端口失败！" << endl;
        return -4;
    }
    long item = 0;
    fd_set rdfs;
    FD_ZERO(&rdfs);
    // 永真循环，接收客户端连接，并发送相应信息  
    while (1) {
        SOCKET sockClt = 0;
        SOCKET MAXSOCKFD = serSock;
        FD_SET(serSock, &rdfs);
        timeval timeout{ 0, 3000 };
        if (select((int)(MAXSOCKFD + 1), &rdfs, NULL, NULL, &timeout) > 0) {
            if (FD_ISSET(serSock, &rdfs) > 0) {
                // 客户端套接字端口信息
                sockaddr_in cliSock = local;
                int len = sizeof(cliSock);
                sockClt = accept(serSock, (sockaddr*)&cliSock, &len);
                item++;
                char __data[8];
                sprintf(__data, "%f", random(1000));
                char clibuf[16];
                sprintf_s(clibuf, __data, addr_out(cliSock.sin_addr), item);
                send(sockClt, clibuf, (int)strlen(clibuf) + 1, 0);
                u_long ul = 0;
                int iResult = ioctlsocket(sockClt, FIONBIO, (unsigned long*)&ul);
                time_t tmt;
                ::time(&tmt);
                struct tm* time = localtime(&tmt);
                time->tm_year += 1900;
                time->tm_mon += 1;
                char timbuf[32];
                sprintf(timbuf, "%d/%d/%d %d:%d:%d", time->tm_year, time->tm_mon, time->tm_mday,
                    time->tm_hour, time->tm_min, time->tm_sec);
                cout << "-----------------------------------------" << endl \
                    << " (" << item << "," << iResult << ")" << clibuf << " [" << timbuf << "]" << endl \
                    << "-----------------------------------------" << endl;
                int lencv = 0;
                do {
                    char rcvbuf[1024];
                    lencv = recv(sockClt, rcvbuf, sizeof(rcvbuf) - 1, 0);
                    if (lencv == SOCKET_ERROR) {
                        cout << "网络异常。" << endl; \
                            continue;
                    } else if (lencv > 0) {
                        printf("已接收数据：");
                        for (int c = 0; c < lencv; c++) {
                            if (c % 32 == 0)
                                printf("\n");
                            printf("%02x ", (unsigned char)rcvbuf[c]);
                        }
                        printf("\n");
                        for (int c = 0; c < lencv; c++) {
                            cout << rcvbuf[c];
                        }
                        cout << endl;
                        if (string(rcvbuf).substr(0, 3) == "GET" || string(rcvbuf).substr(0, 4) == "POST") {
                            closesocket(sockClt);
                            break;
                        } else {
                            continue;
                        }
                    } else if (lencv < 0) {
                        if (errno == EINTR || errno == EWOULDBLOCK) {
                            cout << "慢系统调用(slow system call)" <<
                                ntohl((u_long)inet_addr(inet_ntoa(cliSock.sin_addr))) << endl;
                            continue;
                        } else
                            printf("连接异常，请重试！\n"); // fail
                    } else if (lencv == 0) {
                        cout << "客户端已断开。" << endl;
                    }
                } while (lencv > 0);
            }
            MAXSOCKFD = (MAXSOCKFD > (int)sockClt ? MAXSOCKFD : sockClt);
        }
    }
    // 关闭套接字  
    closesocket(serSock);
    // 撤销WINDOWS SOCKET API  
    WSACleanup();
    return 0;
}

int main(int argc, char* argv[])
{
    //按下ESC键退出
    cout << "Press ESCAPE to exit program\r\n";
    int argp = 0;
    if (argc > 1) {
        argp = atoi(argv[1]);
    }
    //创建处理线程
    _beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))SocketThread, &argp, 0, NULL);
    //阻塞主线程
    while (true) {
        if (getch() == 27)
            break;
        Sleep(100);
    }
    return 0;
}
