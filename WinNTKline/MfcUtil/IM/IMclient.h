#ifndef _IMCHAT_H
#define _IMCHAT_H
#pragma comment(lib, "WS2_32.lib")
#pragma warning (disable:4477)
#pragma warning (disable:4819)

#ifdef _UTILAPIS_
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

typedef struct IMSetting {
    int idx = 0x0;
    char addr[MAX_PATH] = { NULL };
    char IP[16];
	char auth[80];
    int err = -1;
} st_settings;

struct LAST
{
    char lastuser[24] = { NULL };
    char lastgrop[24] = { NULL };
};

typedef struct CLIENT
{
	SOCKET sock = NULL;
	sockaddr_in srvaddr;
    CRITICAL_SECTION wrcon;
	LAST* last;
    void* Dlg;
	char url[64];
	int status = 0;
} st_client;

struct MENU {
    int key; std::string value;
};

struct MSG_trans {
    unsigned char rsv;
    unsigned char cmd;
    unsigned char ret[2];
    unsigned char crc[4];
	union {
		unsigned char usr[24];
		unsigned char hgrp[24];

	};
	union {
		unsigned char psw[24];
		unsigned char grp[24];
		unsigned char jgrp[24];
		unsigned char intro[24];
	};
	unsigned char npsw[24];
};

const MENU menus[] =
{
    { 0x00,"命令菜单" },
    { 0x01,"注册" },
    { 0x02,"登陆" },
    { 0x03,"帮助" },
    { 0x04,"登出" },
    { 0x05,"设置密码" },
    { 0x06,"重新载入" },
    { 0x07,"好友列表" },
    { 0x08,"群" },
    { 0x09,"群成员" },
    { 0x0A,"创建群" },
    { 0x0B,"加入群" },
    { 0x0C,"退群" },
};

enum  em_menu{
    REGIST = 0,
    LOGIN,
    HELP,
    LOGOUT,
    SETPSW,
    RELOAD,
    FRIENDLIST,
    VIEWGROUP,
    MEMBER,
    HOSTGROUP,
    JOINGROUP,
    EXITGROUP,
    CHAT = 0xe,
    TALK,
};

int InitChat(st_settings* setting = NULL);
int SetCommond(unsigned int cmd);
int StartChat(int err, void(*func)(void*));
int SetChatMsg(MSG_trans* msg);
int callbackLog(char* usr, char* psw);
int SettransMsg(MSG_trans* msg);
int SetStatus(int t);
int CloseChat();

#endif
