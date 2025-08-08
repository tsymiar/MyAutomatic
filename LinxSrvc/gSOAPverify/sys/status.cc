#include "status.h"

int get_mem_stat(char* ip, st_sys* sys)
{
    struct in_addr addr;
    struct hostent* hostp;
    if (ip == nullptr || sys == nullptr)
        return -1;
    if (inet_aton(ip, &addr) != 0)
        hostp = gethostbyaddr((const char*)&addr, sizeof(addr), AF_INET);
    else
        hostp = gethostbyname(ip);
    if (hostp != nullptr) {
        sys->s_host = hostp->h_name;
        char** h_list;
        for (h_list = hostp->h_aliases; *h_list != nullptr; h_list++)
            sys->ss_alias = *h_list;
        printf("official hostname: %s\nalias: %s\n", sys->s_host, sys->ss_alias);
        for (h_list = hostp->h_addr_list; *h_list != nullptr; h_list++) {
            addr.s_addr = ((struct in_addr*)*h_list)->s_addr;
            sys->ss_addr = inet_ntoa(addr);
        }
    }
    printf("address: %s\n", sys->ss_addr);
    sys->li_cpu = sysconf(_SC_NPROCESSORS_CONF);  //CPU个数
    long page_size = sysconf(_SC_PAGESIZE);       //系统页面大小bits
    sys->li_page = page_size / 1024;
    long num_pages = sysconf(_SC_PHYS_PAGES);     //系统中物理页数
    long free_pages = sysconf(_SC_PAGE_SIZE);     //系统可用的页面数
    long long  mem = (long long)((long long)num_pages * (long long)page_size);
    mem /= ONE_MB;                                //总物理内存
    sys->mem_all = mem;
    long long  free_mem = (long long)free_pages * (long long)page_size;
    free_mem /= ONE_MB;                           //空闲的物理内存
    sys->mem_free = free_mem;
    printf("CPU:\x20%ld core(s)\nmemory pages:\x20%ldK\ntotal RAM:\x20%lldMB\nFREE RAM:\x20%lldMB(%.3f%%)\n", sys->li_cpu, sys->li_page, sys->mem_all, sys->mem_free, 100.f * sys->mem_free / sys->mem_all);
    return 0;
}

int detect_eth_cable(char* ifname)
{
    struct ifreq ifr;
    struct ethtool_value ethval;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        perror("Cannot get control socket");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, ifname);

    ethval.cmd = 0x0000000A;
    ifr.ifr_data = (caddr_t)&ethval;
    if (ioctl(fd, 0x8946, &ifr) == 0) {
        fprintf(stdout, "Link detecting %s\n", ethval.data ? "OK" : "fail");
    } else if (errno != EOPNOTSUPP) {
        perror("Cannot get link status");
    }
    return(ethval.data == 1 ? 1 : 0);
}
