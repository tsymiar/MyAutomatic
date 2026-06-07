#include "status.h"

int get_mem_stat(const char* ip, st_sys* sys)
{
    if (ip == nullptr || sys == nullptr) {
        return -1;
    }
    // ---------- 主机名/IP 解析 ----------
    struct in_addr addr;
    int rtn = 0;
    if (inet_aton(ip, &addr) != 0) {
        // getnameinfo 替代已弃用的 gethostbyaddr
        struct sockaddr_in sa;
        memset(&sa, 0, sizeof(sa));
        sa.sin_family = AF_INET;
        sa.sin_addr = addr;
        char hostname[NI_MAXHOST] = { 0 };
        rtn = getnameinfo((const struct sockaddr*)&sa, sizeof(sa),
            hostname, sizeof(hostname), NULL, 0, 0);
        if (rtn == 0) {
            sys->s_host = strdup(hostname);
        }
    } else {
        // getaddrinfo 替代已弃用的 gethostbyname
        struct addrinfo hints, * result = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        rtn = getaddrinfo(ip, NULL, &hints, &result);
        if (rtn == 0 && result != nullptr) {
            struct sockaddr_in* sa = (struct sockaddr_in*)result->ai_addr;
            addr.s_addr = sa->sin_addr.s_addr;
            sys->s_host = strdup(ip);  // 保留原始 IP 作为主机名
            sys->ss_addr = strdup(inet_ntoa(addr));
            freeaddrinfo(result);
        }
    }
    printf("address: %s\n", sys->ss_addr ? sys->ss_addr : "unknown");
    // ---------- 内存统计 ----------
    sys->li_cpu = sysconf(_SC_NPROCESSORS_CONF);
    long page_size = sysconf(_SC_PAGESIZE);
    sys->li_page = page_size / 1024;
    long num_pages = sysconf(_SC_PHYS_PAGES);       // 总物理页数
    long free_pages = sysconf(_SC_AVPHYS_PAGES);      // 可用物理页数 (修复: 之前误用 _SC_PAGE_SIZE)

    long long mem = (long long)num_pages * (long long)page_size;
    sys->mem_all = mem / ONE_MB;
    long long free_mem = (long long)free_pages * (long long)page_size;
    sys->mem_free = free_mem / ONE_MB;

    printf("CPU: %ld core(s)\n"
        "memory pages: %ldK\n"
        "total RAM: %lldMB\n"
        "FREE RAM: %lldMB (%.3f%%)\n",
        sys->li_cpu, sys->li_page, sys->mem_all, sys->mem_free,
        sys->mem_all > 0 ? (100.f * sys->mem_free / sys->mem_all) : 0.f);
    return 0;
}

int detect_eth_cable(const char* ifname)
{
    if (ifname == nullptr) {
        return -1;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Cannot get control socket");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    struct ethtool_value ethval;
    ethval.cmd = ETHTOOL_GLINK_VAL;
    ifr.ifr_data = (caddr_t)&ethval;

    if (ioctl(fd, SIOCETHTOOL_CMD, &ifr) == 0) {
        fprintf(stdout, "Link detecting %s\n", ethval.data ? "OK" : "fail");
        close(fd);
        return (ethval.data == 1 ? 1 : 0);
    }
    if (errno != EOPNOTSUPP) {
        perror("Cannot get link status");
    }
    close(fd);
    return -1;
}
