#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define DEVICE_NAME "phy_mem_drv"
#define IOCTL_MAGIC 0x1234

#define IOCTL_SET_PHY_ADDR   _IOW(IOCTL_MAGIC, 1, struct phy_addr_params)
#define IOCTL_SET_OUTPUT_FD  _IOW(IOCTL_MAGIC, 2, int)
#define IOCTL_WRITE_CHUNK    _IOW(IOCTL_MAGIC, 3, struct chunk_params)
#define CHUNK_SIZE (1024 * 1024)

struct phy_addr_params {
    unsigned long phys_addr;
    size_t total_size;
};

struct chunk_params {
    size_t mem_offset;
    size_t chunk_size;
    size_t file_offset;
};

unsigned long getUsecTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000ULL + tv.tv_usec);
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <output_file> <phy_addr in hex> <total_size in bytes>\n", argv[0]);
        return -1;
    }

    const char* filename = argv[1];
    unsigned long phy_addr = strtoul(argv[2], NULL, 16);
    size_t total_size = strtoul(argv[3], NULL, 10);

    int dev_fd, file;
    struct phy_addr_params phy_params = {phy_addr, total_size};
    struct chunk_params chunk_params;

    dev_fd = open("/dev/phy_mem_drv", O_RDWR);
    if (dev_fd < 0) {
        perror("打开设备失败");
        return -1;
    }

    if (ioctl(dev_fd, IOCTL_SET_PHY_ADDR, &phy_params) < 0) {
        perror("设置物理地址失败");
        close(dev_fd);
        return -1;
    }

    file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file < 0) {
        perror("打开输出文件失败");
        close(dev_fd);
        return -1;
    }

    if (ioctl(dev_fd, IOCTL_SET_OUTPUT_FD, &file) < 0) {
        perror("设置文件句柄失败");
        close(file);
        close(dev_fd);
        return -1;
    }
    close(file);

    unsigned long start = getUsecTime();
    for (size_t offset = 0; offset < phy_params.total_size; offset += CHUNK_SIZE) {
        chunk_params.mem_offset = offset;
        chunk_params.file_offset = offset;
        chunk_params.chunk_size = (phy_params.total_size - offset) < CHUNK_SIZE ?
                                  (phy_params.total_size - offset) : CHUNK_SIZE;

        if (ioctl(dev_fd, IOCTL_WRITE_CHUNK, &chunk_params) < 0) {
            perror("写入数据块失败");
            break;
        }
    }
    printf("%lu 字节已写入，速度为 %.3f M/s.\n",
           phy_params.total_size,
           (phy_params.total_size * 1.f) / (getUsecTime() - start) * 0x100000 / 1000000.f);

    close(dev_fd);
    return 0;
}
