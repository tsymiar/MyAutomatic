#ifndef SYS_STATUS_H
#define SYS_STATUS_H
#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <bits/types.h>

#define	 ONE_MB (1024 * 1024)

#ifdef __cplusplus
extern "C"
{
#endif
	struct ethtool_value
	{
		__uint32_t cmd;
		__uint32_t data;
	};
	struct st_sys
	{
		char* s_host;
		char* ss_alias;
		char* ss_addr;
		long int l_cpu;
		long int l_page;
		long long mem_all;
		long long mem_free;
	};
	int show_memory(char* ip, st_sys* sys);
	int detect_eth_cable(char *ifname);
#ifdef __cplusplus
}
#endif

#endif // SYS_STATUS_H

