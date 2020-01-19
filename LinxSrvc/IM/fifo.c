#include <stdio.h>
#include <errno.h>
//#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

struct Fifo {
    int cmd;
    long long flag;
    void* addr;
    char a[];
};

const char* FIFO_FILE = "/tmp/fifo1";

int write_fifo(const struct Fifo* fifo)
{
    int fdio;
    int fifoSize = sizeof(struct Fifo);
    unsigned long fileSize = -1;
    struct stat statst;

    if (fifo == NULL) {
        perror("Param fifo is NULL");
        return -1;
    }
    printf("FIFO write: Process flag = %lld, command = %d, address = %p.\n", fifo->flag, fifo->cmd, fifo->addr);
    if (stat(FIFO_FILE, &statst) < 0 || access(FIFO_FILE, F_OK) != 0) {
        fprintf(stderr, "Fifo file does not exist.\n");
    } else {
        fileSize = statst.st_size;
        if (fileSize % fifoSize != 0) {
            fprintf(stdout, "File format may invalid.\n");
        }
    }
    if (mkfifo(FIFO_FILE, 0666) < 0 && errno != EEXIST) {
        fprintf(stderr, "Get '%s' size failed.\n", FIFO_FILE);
        return -3;
    }
    if ((fdio = open(FIFO_FILE, O_WRONLY | O_NONBLOCK)) < 0
        && fdio != -1 && errno != ENXIO) {
        perror("Open FIFO O_WRONLY fail");
        return -4;
    }
    if (fileSize > 0)
        lseek(fdio, 0, SEEK_END);
    if (write(fdio, (void*)fifo, fifoSize) < 0) {
        perror("Write FIFO fail");
        close(fdio);
        return -5;
    }
    if (fileSize > 0)
        lseek(fdio, 0, SEEK_SET);
    close(fdio);
    return 0;
}

struct Fifo read_fifo(long long flag)
{
    int fdio;
    ssize_t len;
    int fifoSize = sizeof(struct Fifo);
    struct stat statst;
    struct Fifo fifo = {
        .cmd = -1,
        .flag = -1,
        .addr = NULL
    };
    if (flag == -1)
        return fifo;
    if (stat(FIFO_FILE, &statst) < 0) {
        perror("Stat FIFO fail");
        return fifo;
    }
    if ((fdio = open(FIFO_FILE, O_RDONLY)) < 0
        && fdio != -1 && errno != ENXIO) {
        perror("Open FIFO RDONLY fail");
        return fifo;
    }
    while ((len = read(fdio, &fifo, fifoSize)) > 0) {
        if (fifo.flag == flag) {
            break;
        } else {
            printf("FIFO matching: Process flag = %lld, command = %d, address = %p.\n", fifo.flag, fifo.cmd, fifo.addr);
            fifo.flag = -1;
            write(fdio, (void*)'\0', 1);
            fdio = open(FIFO_FILE, O_RDONLY);
            continue;
        }
    }
    close(fdio);
    return fifo;
}

unsigned int sIP2i(const char* IP)
{
    unsigned int ip = 0;
    const char* s = IP;
    unsigned char t = 0;
    while (1) {
        if (*s != '\0' && *s != '.') {
            t = (unsigned char)(t * 10 + *s - '0');
        } else {
            ip = (ip << 8) + t;
            if (*s == '\0')
                break;
            t = 0;
        }
        s++;
    };
    return ip;
}

int main(int argc, const char* argv[])
{
    long long flag = (9001 << 16 | 8080 << 8 | sIP2i("127.0.0.1"));
    struct Fifo fifo0 = {
        .cmd = -1,
        .flag = flag,
        .addr = NULL };
    //std::thread th([](long long flag) {
    struct Fifo fifo = read_fifo(flag);
    printf("FIFO read: Process flag = %lld, command = %d, address = %p.\n", fifo.flag, fifo.cmd, fifo.addr);
    //    }, flag);
    //th.detach();
    write_fifo(&fifo0);
    usleep(1000);
    return 0;
}
