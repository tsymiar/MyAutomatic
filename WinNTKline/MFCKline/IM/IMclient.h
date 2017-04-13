#ifndef IM_IMCHAT_H
#define IM_IMCHAT_H
#pragma comment(lib, "WS2_32.lib")

#ifdef _TESTDLG_
// /clr模式包含该头文件将会导致
// error LNK2022 : 元数据操作失败(8013118D) : 重复类型(group_filter)中的布局信息不一致 : (0x0200020b)。
// 2>LINK : fatal error LNK1255 : 由于元数据错误，链接失败
#include <WS2tcpip.h>
#else
#include <winsock2.h>
#endif
#include <process.h>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <map>
#pragma warning (disable:4477)

typedef struct IMUSR {
	char addr[MAX_PATH];
	char comm[64];
	char psw[32];
	char usr[32];
	int err;
} st_imusr;

struct CHATMSG
{
	char lastuser[256];
	char lastgroup[256];
};

struct LPR
{
	SOCKET sock;
	CRITICAL_SECTION wrcon;
	void* dlg;
	CHATMSG *msg;
};

int InitChat(char argv[] = "127.0.0.1" , int argc = 2);
int StartChat(int err, void(*func)(void*));
void CloseChat();

#endif