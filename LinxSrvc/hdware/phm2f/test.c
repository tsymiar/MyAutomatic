#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdint.h>

#define DEVICE_NAME "/dev/phy_mem_drv"
#define IOCTL_MAGIC 0x12347379

#define IOCTL_SET_PHY_ADDR   _IOW(IOCTL_MAGIC, 1, struct phy_addr_params)
#define IOCTL_ADD_USER_FD    _IOW(IOCTL_MAGIC, 2, int)
#define IOCTL_WRITE_CHUNK    _IOW(IOCTL_MAGIC, 3, struct chunk_params)
#define IOCTL_CLOSE_FILE     _IOW(IOCTL_MAGIC, 4, int)
#define IOCTL_SYNC_FILE      _IOW(IOCTL_MAGIC, 5, int)
#define CHUNK_SIZE (1024 * 1024)

typedef uint64_t phys_addr_t;

struct phy_addr_params {
    phys_addr_t phys_addr;
    unsigned long total_size;
};

struct chunk_params {
    int usr_id;
    size_t mem_offset;
    size_t chunk_size;
    size_t file_offset;
};

static unsigned long getUsecTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000ULL + tv.tv_usec);
}

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <hex phys_addr> <hex total_size> [hex chunk_size] [max files]\n", argv[0]);
        return -1;
    }

    struct phy_addr_params phy_params;
    phy_params.phys_addr = strtoul(argv[1], NULL, 16);
    phy_params.total_size = strtoul(argv[2], NULL, 16);
    if (phy_params.phys_addr == 0 || phy_params.total_size == 0) {
        fprintf(stderr, "物理地址或总大小不合法: %lu, %lu.\n", phy_params.phys_addr, phy_params.total_size);
        return -1;
    }

    unsigned long chunk_size = CHUNK_SIZE;
    if (argc > 3) {
        chunk_size = strtoul(argv[3], NULL, 16);
        if (chunk_size == 0 || chunk_size > phy_params.total_size) {
            fprintf(stderr, "单次写入长度不合法: %lu.\n", chunk_size);
            return -1;
        }
    }
    int maxfiles = 1;
    if (argc > 4) {
        maxfiles = strtoul(argv[4], NULL, 10);
    }

    int dev_fd = open(DEVICE_NAME, O_RDWR);
    if (dev_fd < 0) {
        perror("打开设备节点("DEVICE_NAME")失败");
        return -1;
    }

    if (ioctl(dev_fd, IOCTL_SET_PHY_ADDR, &phy_params) < 0) {
        perror("设置物理地址失败");
        close(dev_fd);
        return -1;
    }

    for (int i = 0; i < maxfiles; i++) {
        char filename[128];
        snprintf(filename, sizeof(filename), "file%d.bin", i);
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("打开输出文件失败");
            close(dev_fd);
            return -1;
        }
        if (ioctl(dev_fd, IOCTL_ADD_USER_FD, &fd) < 0) {
            perror("设置文件句柄失败");
            close(dev_fd);
            return -1;
        }
        close(fd);
    }

    int fids[maxfiles];
    unsigned long start = getUsecTime();
    for (int i = 0; i < maxfiles; i++) {
        for (size_t offset = 0; offset < phy_params.total_size; offset += chunk_size) {
            struct chunk_params chunk_params;
            chunk_params.mem_offset = offset;
            chunk_params.file_offset = offset;
            chunk_params.chunk_size = (phy_params.total_size - offset) < chunk_size ?
                (phy_params.total_size - offset) : chunk_size;
            if (ioctl(dev_fd, IOCTL_WRITE_CHUNK, &chunk_params) < 0) {
                perror("写入数据块失败");
                break;
            }
            fids[i] = chunk_params.usr_id;
        }
    }
    printf("%lu 字节已写入，速度为 %.3f M/s.\n",
        phy_params.total_size,
        (phy_params.total_size * 1.f) / (getUsecTime() - start) * 0x100000 / 1000000.f);


    for (int i = 0; i < maxfiles; i++) {
        if (ioctl(dev_fd, IOCTL_SYNC_FILE, &fids[i]) < 0) {
            perror("文件刷盘失败");
            close(dev_fd);
            return -1;
        }
        if (ioctl(dev_fd, IOCTL_CLOSE_FILE, &fids[i]) < 0) {
            perror("文件关闭失败");
            close(dev_fd);
            return -1;
        }
    }
    close(dev_fd);
    return 0;
}
