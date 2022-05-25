#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

struct PipeFifo {
    int value;
    long long flag;
    void* reserve;
    char a[0];
};

const char* FIFO_FILE = "/tmp/fifo1";
static int filedes[2];

int write_pipe(const struct PipeFifo _pipe)
{
    if (pipe(filedes) < 0) {
        perror("make pipe fail");
        return -1;
    }
    close(filedes[0]);
    if (write(filedes[1], &_pipe, sizeof(struct PipeFifo)) < 0) {
        perror("write pipe to filedes[1] fail");
    }
    return 0;
}

struct PipeFifo read_pipe(struct PipeFifo* pipe)
{
    close(filedes[1]);
    if (read(filedes[0], pipe, sizeof(struct PipeFifo)) < 0) {
        perror("read pipe fail from filedes[0]");
    }
    return *pipe;
}

int write_fifo(const struct PipeFifo* fifo)
{
    int fio;
    int size = sizeof(struct PipeFifo);
    unsigned long fileSize = -1;
    struct stat status;

    if (fifo == NULL) {
        fprintf(stderr, "fifo pointer is NULL\n");
        return -1;
    }
    if (stat(FIFO_FILE, &status) < 0 || access(FIFO_FILE, F_OK) != 0) {
        fprintf(stderr, "fifo file does not exist.\n");
    } else {
        fileSize = status.st_size;
        if (fileSize % size != 0) {
            fprintf(stdout, "file format may invalid.\n");
        }
    }
    if (mkfifo(FIFO_FILE, 0666) < 0 && errno != EEXIST) {
        fprintf(stderr, "get '%s' size failed.\n", FIFO_FILE);
        return -3;
    }
    if ((fio = open(FIFO_FILE, O_WRONLY | O_NONBLOCK)) < 0
        && fio != -1 && errno != ENXIO) {
        perror("open FIFO by O_WRONLY fail");
        return -4;
    }
    if (fileSize > 0)
        lseek(fio, 0, SEEK_END);
    if (write(fio, (void*)fifo, size) < 0) {
        perror("write FIFO fail");
        close(fio);
        return -5;
    }
    if (fileSize > 0)
        lseek(fio, 0, SEEK_SET);
    close(fio);
    return 0;
}

struct PipeFifo read_fifo(long long flag)
{
    int fio;
    ssize_t len;
    int size = sizeof(struct PipeFifo);
    struct stat status;
    struct PipeFifo fifo = {
        .value = -1,
        .flag = -1,
        .reserve = NULL
    };
    if (flag == -1) return fifo;
    if (stat(FIFO_FILE, &status) < 0) {
        fprintf(stderr, "stat fifo fail");
        return fifo;
    }
    if ((fio = open(FIFO_FILE, O_RDONLY)) < 0
        && fio != -1 && errno != ENXIO) {
        fprintf(stderr, "open FIFO by RDONLY fail");
        return fifo;
    }
    while ((len = read(fio, &fifo, size)) > 0) {
        if (fifo.flag == flag) {
            break;
        } else {
            fprintf(stderr, "FIFO mismatch: { %d, %lld, %p }.\n", fifo.value, fifo.flag, fifo.reserve);
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
    const char* hint = "Usage:\n./pipefifo [cmd]\n -- pipe: 1(w)/0(r)\n -- fifo: 2(r)/3(w)\n";
    if (flag < 0) {
        printf("%s\n", hint);
        return 0;
    }
    struct PipeFifo pp0ff = {
        .value = -1,
        .flag = flag,
        .reserve = NULL
    };
    int stat = -1;
    if (flag == 0) {
        pp0ff = read_pipe(&pp0ff);
        printf("PIPE reads: { %d, %lld, %p }.\n", pp0ff.value, pp0ff.flag, pp0ff.reserve);
    } else if (flag == 1) {
        stat = write_pipe(pp0ff);
        printf("PIPE wrote status = %d.\n", stat);
    } else if (flag == 2) {
        pp0ff = read_fifo(flag);
        printf("FIFO reads: { %d, %lld, %p }.\n", pp0ff.value, pp0ff.flag, pp0ff.reserve);
    } else if (flag == 3) {
        stat = write_fifo(&pp0ff);
        printf("FIFO wrote status = %d.\n", stat);
    } else {
        printf("%s\n", hint);
        return 0;
    }
    return 0;
}
