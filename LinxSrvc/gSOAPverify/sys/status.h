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
#include <sys/socket.h>

#define ONE_MB (1024 * 1024)

// 主机名 / IP 直接用定长数组保存，调用方无需 free，也不会漏内存
#define SYS_HOST_LEN (256)
#define SYS_ADDR_LEN (16)

// SIOCETHTOOL ioctl 命令码 (linux/sockios.h 未直接暴露)
#define SIOCETHTOOL_CMD   0x8946
// ETHTOOL_GLINK: 获取网卡链路状态
#define ETHTOOL_GLINK_VAL 0x0000000A

#ifdef __cplusplus
extern "C" {
#endif

    struct ethtool_value {
        __uint32_t cmd;
        __uint32_t data;
    };

struct st_sys {
    long int  li_cpu;                  // CPU 核心数
    long int  li_page;                 // 页面大小 (KB)
    long long mem_all;                 // 总物理内存 (MB)
    long long mem_free;                // 空闲物理内存 (MB)
    char      s_host[SYS_HOST_LEN];    // 主机名
    char      ss_alias[SYS_HOST_LEN];  // 主机别名
    char      ss_addr[SYS_ADDR_LEN];   // IP 地址
};

int get_mem_stat(const char* ip, st_sys* sys);
int detect_eth_cable(const char* ifname);

#ifdef __cplusplus
}
#endif

#endif // SYS_STATUS_H

