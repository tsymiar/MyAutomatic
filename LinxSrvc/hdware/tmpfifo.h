#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

struct Fifo {
    int cmd;
    long long flag;
    void* addr;
    char a[0];
};

const char* FIFO_FILE = "/tmp/fifo1";

int write_fifo(const struct Fifo* fifo)
{
    int fio;
    int size = sizeof(struct Fifo);
    unsigned long fileSize = -1;
    struct stat status;

    if (fifo == NULL) {
        perror("Param fifo is NULL");
        return -1;
    }
    if (stat(FIFO_FILE, &status) < 0 || access(FIFO_FILE, F_OK) != 0) {
        fprintf(stderr, "Fifo file does not exist.\n");
    } else {
        fileSize = status.st_size;
        if (fileSize % size != 0) {
            fprintf(stdout, "File format may invalid.\n");
        }
    }
    if (mkfifo(FIFO_FILE, 0666) < 0 && errno != EEXIST) {
        fprintf(stderr, "Get '%s' size failed.\n", FIFO_FILE);
        return -3;
    }
    if ((fio = open(FIFO_FILE, O_WRONLY | O_NONBLOCK)) < 0
        && fio != -1 && errno != ENXIO) {
        perror("Open FIFO O_WRONLY fail");
        return -4;
    }
    if (fileSize > 0)
        lseek(fio, 0, SEEK_END);
    if (write(fio, (void*)fifo, size) < 0) {
        perror("Write FIFO fail");
        close(fio);
        return -5;
    }
    if (fileSize > 0)
        lseek(fio, 0, SEEK_SET);
    close(fio);
    return 0;
}

struct Fifo read_fifo(long long flag)
{
    int fio;
    ssize_t len;
    int size = sizeof(struct Fifo);
    struct stat status;
    struct Fifo fifo = {
        .cmd = -1,
        .flag = -1,
        .addr = NULL
    };
    if (flag == -1) return fifo;
    if (stat(FIFO_FILE, &status) < 0) {
        perror("Stat FIFO fail");
        return fifo;
    }
    if ((fio = open(FIFO_FILE, O_RDONLY)) < 0
        && fio != -1 && errno != ENXIO) {
        perror("Open FIFO RDONLY fail");
        return fifo;
    }
    while ((len = read(fio, &fifo, size)) > 0) {
        if (fifo.flag == flag) {
            break;
        } else {
            printf("FIFO message: { %d, %lld, %p }.\n", fifo.cmd, fifo.flag, fifo.addr);
            fifo.flag = -1;
            write(fio, (void*)'\0', 1);
            fio = open(FIFO_FILE, O_RDONLY);
            continue;
        }
        usleep(1000);
    }
    close(fio);
    return fifo;
}

int fifo_test(long long flag)
{
    const char* hint = "Usage:\n./tmpfifo [1(w)/0(r)]";
    if (flag < 0) {
        printf("%s\n", hint);
        return 0;
    }
    struct Fifo fi0 = {
        .cmd = -1,
        .flag = flag,
        .addr = NULL };
    struct Fifo fifo;
    if (flag == 0) {
        fifo = read_fifo(flag);
    } else if (flag == 1) {
        write_fifo(&fi0);
        printf("FIFO wrote: { %d, %lld, %p }.\n", fifo.cmd, fifo.flag, fifo.addr);
    } else {
        printf("%s\n", hint);
        return 0;
    }
    return 0;
}
