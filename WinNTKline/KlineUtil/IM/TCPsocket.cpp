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
    cout << "Starting up TCP: " << port << endl;

    WSADATA wsa; //用于WSAstartup填充
    int err = WSAStartup(0x101, &wsa); //WSAstartup初始化WINDOWS SOCKET API

    if (err != 0) {
        cout << "初始化套接字失败！" << endl;
        return -1;
    }
    //重置字节流套接口
    if (LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1) {
        WSACleanup();
        return 0;
    }
    //设置服务器端套接字
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);//基于TCP
    if (sockSrv == INVALID_SOCKET) {
        cout << "无效的套接字！" << endl;
        return -1;
    }
    //描述本地信息
    sockaddr_in addrSrv; //本地端口信息
    addrSrv.sin_addr.S_un.S_addr = INADDR_ANY;//接受所有连接
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons((u_short)port); //本地服务器端口号
    int optval = 1;
    //检测对方主机是否崩溃，避免（服务器）永远阻塞于TCP连接的输入。
    setsockopt(sockSrv, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval));
    cout << "绑定..." << endl;//绑定
    if (bind(sockSrv, (sockaddr*)&addrSrv, sizeof(addrSrv)) != 0) {
        closesocket(sockSrv);
        cout << "绑定端口失败，端口被占用！" << endl;
        return -1;
    }
    //监听
    cout << "监听..." << endl;
    if (listen(sockSrv, 5) != 0) {
        closesocket(sockSrv);
        cout << "监听端口失败！" << endl;
        return -1;
    }
    sockaddr_in cliSock = addrSrv; //客户端端口信息
    SOCKET MAXSOCKFD = sockSrv;
    int len = sizeof(cliSock);
    u_long ul = 0;
    fd_set rdfs;
    FD_ZERO(&rdfs);
    //永真循环，接收客户端连接，并发送相应信息  
    while (1) {
        FD_SET(sockSrv, &rdfs);
        char __data[8];
        sprintf(__data, "%f", random(1000));
        timeval timeout{ 3, 0 };
        if (select((int)(MAXSOCKFD + 1), &rdfs, NULL, NULL, &timeout) > 0) {
            int item = 0;
            SOCKET sockClt;
            if (FD_ISSET(sockSrv, &rdfs) > 0) {
                char clibuf[32];
                //客户端套接字
                sockClt = accept(sockSrv, (sockaddr*)&cliSock, &len);
                sprintf_s(clibuf, __data, addr_out(cliSock.sin_addr), item);
                cout << "---------------------------------" << endl \
                    << "(" << item << ")" << clibuf << endl \
                    << "---------------------------------" << endl;
                send(sockClt, clibuf, (int)strlen(clibuf) + 1, 0);
                int iResult = ioctlsocket(sockClt, FIONBIO, (unsigned long*)&ul);
                char buff[1024];
                int lencv = recv(sockClt, buff, sizeof(buff) - 1, 0);
                if (lencv == SOCKET_ERROR) {
                    cout << "网络异常。" << endl; \
                        continue;
                } else if (lencv > 0) {
                    printf("socket connect sucess\n");
                    for (int c = 0; c < lencv; c++) {
                        if (c % 32 == 0)
                            printf("\n");
                        printf("%02x ", (unsigned char)buff[c]);
                    }
                    printf("\n");
                    continue;
                } else if (lencv < 0) {
                    if (errno == EINTR || errno == EWOULDBLOCK) {
                        cout << "慢系统调用(slow system call)" <<
                            ntohl((u_long)inet_addr(inet_ntoa(cliSock.sin_addr))) << endl;
                        continue;
                    } else
                        printf("socket disconnected!connect again!\n"); // fail
                } else if (lencv == 0) {
                    cout << "客户端已断开。" << endl;
                }
                item++;
            }
            MAXSOCKFD = (int)(MAXSOCKFD > (int)sockClt ? MAXSOCKFD : sockClt);
        }
    }
    //关闭套接字  
    closesocket(sockSrv);
    //撤销WINDOWS SOCKET API  
    WSACleanup();
    return 0;
}

int main(int argc, char* argv[])
{
    //按下ESC键退出
    cout << "Press ESCAPE to teminate program\r\n";
    int argi = 0;
    if (argc > 1) {
        argi = atoi(argv[1]);
    }
    //创建处理线程
    _beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))SocketThread, &argi, 0, NULL);
    //阻塞主线程
    while (true) {
        if (getch() == 27)
            break;
    }
    return 0;
}
