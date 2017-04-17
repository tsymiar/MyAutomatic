#ifndef IM_IMCHAT_H
#define IM_IMCHAT_H
#pragma comment(lib, "WS2_32.lib")

#ifdef _TESTDLG_
// /clr模式包含该头文件将会导致
// error LNK2022 : 元数据操作失�?8013118D) : 重复类型(group_filter)中的布局信息不一�?: (0x0200020b)�?
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
using namespace std;

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

struct CMD {
	int idx; string val;
};

const CMD cmds[] =
{
	{ 0x00,"命令菜单" },
	{ 0x01,"注册" },
	{ 0x02,"登陆" },
	{ 0x03,"登出" },
	{ 0x04,"帮助" },
	{ 0x05,"好友列表" },
	{ 0x06,"�? },
	{ 0x07,"群成�? },
	{ 0x08,"设置描述" },
	{ 0x09,"查询描述" },
	{ 0x0A,"设置密码" },
	{ 0x0B,"创建�? },
	{ 0x0C,"加入�? },
	{ 0x0D,"退�? },
};

int InitChat(char argv[] = "127.0.0.1", int argc = 2);
int StartChat(int err, void(*func)(void*));
int SetChatCmd(unsigned int opt);
void CloseChat();

#endif