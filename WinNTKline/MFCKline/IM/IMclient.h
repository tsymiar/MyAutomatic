#ifndef IM_IMCHAT_H
#define IM_IMCHAT_H
#pragma comment(lib, "WS2_32.lib")

#include <winsock2.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <conio.h>

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