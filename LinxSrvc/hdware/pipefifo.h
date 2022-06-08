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

void open_pipe()
{
    if (pipe(filedes) < 0) {
        perror("make pipe fail");
        return;
    }
}

int write_pipe(const struct PipeFifo _pipe)
{
    close(filedes[0]);
    if (write(filedes[1], &_pipe, sizeof(struct PipeFifo)) < 0) {
        perror("write pipe to filedes[1] fail");
    }
    return 0;
}

struct PipeFifo read_pipe(struct PipeFifo* _pipe)
{
    struct PipeFifo pipe = { 0, -1, 0x0 };
    if (_pipe == NULL) {
        fprintf(stderr, "fifo pointer is NULL\n");
        return pipe;
    }
    close(filedes[1]);
    read(filedes[0], _pipe, sizeof(struct PipeFifo));
    pipe = *_pipe;
    return pipe;
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
            fprintf(stderr, "FIFO catch: { %d, %lld, %p }.\n", fifo.value, fifo.flag, fifo.reserve);
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

int pipe_fifo_test(long long flag, int value)
{
    const char* hint = "Usage:\n./pipefifo [cmd] [val]\n cmds:\n -- pipe: 0\n -- fifo: 1(r)/2(w)";
    if (flag < 0) {
        printf("%s\n", hint);
        return -1;
    }
    struct PipeFifo pp0ff = {
        .value = value,
        .flag = flag,
        .reserve = NULL
    };
    int stat = -1;
    if (flag == 0) {
        open_pipe();
        pid_t pid = fork();
        if (pid == 0) {
            stat = write_pipe(pp0ff);
            printf("PIPE wrote: { %d, %lld, %p }\n", pp0ff.value, pp0ff.flag, pp0ff.reserve);
            exit(0);
        } else if (pid > 0) {
            pp0ff = read_pipe(&pp0ff);
            stat = pp0ff.flag;
            printf("PIPE reads: { %d, %lld, %p }, ", pp0ff.value, pp0ff.flag, pp0ff.reserve);
        } else {
            perror("fork child fail");
        }
    } else if (flag == 1) {
        pp0ff = read_fifo(flag);
        stat = pp0ff.flag;
        printf("FIFO reads: { %d, %lld, %p }, ", pp0ff.value, pp0ff.flag, pp0ff.reserve);
    } else if (flag == 2) {
        stat = write_fifo(&pp0ff);
        printf("FIFO wrote ");
    } else {
        printf("%s\n", hint);
        return 1;
    }
    printf("stat = %d.\n", stat);
    return 0;
}
