#pragma once
#include <stdio.h>
#include <iostream>
#include <process.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>
#include "conio.h"
#define random(x) (rand()%x*0.001f)
#pragma comment( lib, "ws2_32.lib" )  
#ifdef _DEBUG  
#define new DEBUG_NEW  
#endif  
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define _WINSOCK_DEPRECATED_NO_WARNINGS    
#pragma warning (disable:4996)
//#define addr_out(ip) inet_ntop(ip)
#define addr_out(ip) inet_ntoa(ip)

using namespace std;

int _ExitFlag = 0;
int opt = 1;
int sock = 0;
int item = 0;
int rtn = 0;
int lencv = 0;
int port = 6001;
char clibuf[32];
char temp[1024];
//char buff[32] = { '\0' };
char* ok = "socket connected\n";
char* fail = "socket disconnected!connect again!\n";
SOCKET client; //客户端套接字  
SOCKET sockSrv; //服务器端套接字
WSADATA wsa; //用于WSAstartup填充  
sockaddr_in addrSrv; //本地端口信息  
timeval timeout{ 3, 0 };
fd_set rdfs;
char __data[8];
//struct tcp_info tcpinfo;
///////////////////////////////////////////////
unsigned int __stdcall SocketThread(void* lp)
{
    cout << "type port:" << endl;
    //while (cin >> port/*, port != '\n'*/)
    //    cout << port << endl;
    //scanf("%d", &port);
    cout << port << endl;
    srand((int)time(0));
    FD_ZERO(&rdfs);
    FD_SET(sock, &rdfs);
    cout << "Starting up TCP sever" << endl;

    int err = WSAStartup(0x101, &wsa); //WSAstartup初始化WINDOWS SOCKET API  

    if (err != 0)
    {
        cout << "初始化套接字失败！" << endl;
        return -1;
    }
    //重置字节流套接口
    if (LOBYTE(wsa.wVersion) != 1 || HIBYTE(wsa.wVersion) != 1)
    {
        WSACleanup();
        return 0;
    }
    //设置Socket
    sockSrv = socket(AF_INET, SOCK_STREAM, 0);//基于TCP
    if (sockSrv == INVALID_SOCKET)
    {
        cout << "无效的套接字！" << endl;
        return -1;
    }
    //描述本地信息  
    addrSrv.sin_addr.S_un.S_addr = INADDR_ANY;//接受所有连接
    addrSrv.sin_family = AF_INET;
    addrSrv.sin_port = htons((u_short)port); //本地服务器端口号  
    setsockopt(sockSrv, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));//检测对方主机是否崩溃，避免（服务器）永远阻塞于TCP连接的输入。
                                                                            //绑定
    cout << "绑定..." << endl;
    if (bind(sockSrv, (sockaddr*)&addrSrv, sizeof(addrSrv)) != 0)
    {
        closesocket(sockSrv);
        cout << "绑定端口失败，端口可能被占用！" << endl;
        return -1;
    }
    //监听
    cout << "监听..." << endl;
    if (listen(sockSrv, 5) != 0)
    {
        closesocket(sockSrv);
        cout << "监听端口失败！" << endl;
        return -1;
    }
    sockaddr_in cliSock = addrSrv; //客户端端口信息  
    int len = sizeof(cliSock);
    //永真循环，接收客户端连接，并发送相应信息  
    while (1)
    {
        sprintf(__data, "%f", random(1000));
        client = accept(sockSrv, (sockaddr*)&cliSock, &len);
        //sprintf_s(temp, "YOUR Client ip is %s\n*****%d", addr_out(cliSock.sin_addr), item);
        sprintf_s(temp, __data, addr_out(cliSock.sin_addr), item);
        send(client, temp, strlen(temp) + 1, 0);
        if (item == 1)
            cout << "---------------------------------" << endl << temp << endl \
            << "---------------------------------" << endl;
        itoa(item, clibuf, 10);
        send(client, clibuf, strlen(clibuf) + 1, 0);
        if (recv(client, temp - '\0', 50, 0) == SOCKET_ERROR)
        {
            rtn = 0;
            cout << "客户端已断开连接。" << endl; \
                continue;// break;
        }
        else if (rtn == 0)
        {
            cout << "客户端连接成功" << endl;
            rtn = 0;
        }
        item++;
        rtn++;
        /*
        rtn = select(sock + 1, &rdfs, NULL, NULL, &timeout);
        if (rtn > 0)
        {
            lencv = recv(sock, temp, sizeof(temp), 0);
            if (lencv > 0)
            {
                printf(ok);
                return 1;
            }
            else if (lencv < 0) {
                if (errno == EINTR) {
                    printf(ok);
                    return 1;
                }
                else
                {
                    printf(fail);
                    return 0;
                }
            }
            else if (lencv == 0) {
                printf(fail);
                return 0;
            }
        }
        else if (rtn == 0)
        {
            //time out
            printf(ok);
            return 1;
        }
        else if (rtn < 0)
        {
            if (errno == EINTR)
            {
                printf(ok);
                return 1;
            }
            else {
                printf(fail);
                return 0;
            }
        }
        cout << "--------------------" << endl << ntohl((u_long)inet_addr(inet_ntoa(cliSock.sin_addr))) << endl;
        */
    }
    //关闭套接字  
    closesocket(sockSrv);
    //撤销WINDOWS SOCKET API  
    WSACleanup();
    return 0;
}

int _tmain_(int argc, _TCHAR* argv[])
{
    int nRetCode = 0;
    //按下ESC键退出  
    cout << "Press ESCAPE to teminate program\r\n";
    //创建处理线程  
    _beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))SocketThread, (void*)0, 0, NULL);
    item = 0;
    /* Just doing something */
    //阻塞主线程
    while (true)
    {
        if (getch() == 27)
            break;
    }
    return nRetCode;
}
