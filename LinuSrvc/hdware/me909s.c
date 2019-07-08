#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <errno.h>
#include <termios.h>

#define MAX_BUF_LEN	1024

int main(int argc, int **argv)
{
    fd_set rdfds;
    FD_ZERO(&rdfds);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500;
    int me_fd, ret, rd_stdin;
    int flag = 0;
    int times = 0;
    char buff[MAX_BUF_LEN], rply[MAX_BUF_LEN];
    struct termios options;

    me_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (me_fd == -1) {
        perror("Can't open /dev/ttyUSB0 - \n");
        return 0;
    } else {
        fcntl(me_fd, F_SETFL, 0);
    }

    bzero(&options, sizeof(options));

    tcgetattr(me_fd, &options);
    //位掩码方式激活本地连接、使能接收
    options.c_cflag |= (CLOCAL | CREAD);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag &= ~CSIZE;
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CSTOPB;
    options.c_iflag |= IGNPAR;
    tcflush(me_fd, TCIFLUSH);

    options.c_oflag = 0;
    options.c_lflag = 0;

    tcflush(me_fd, TCIOFLUSH);
    if (0 == tcsetattr(me_fd, TCSANOW, &options)) {
        printf("Serial tty is ready...\n");
    }

    memset(buff, 0, sizeof(buff));
    strcpy(buff, "AT+CPIN?\r\n");

    while (!flag)
    {
        write(me_fd, buff, sizeof(buff));
        ret = select(me_fd + 1, &rdfds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            break;
        } else if (ret == 0) {
            times++;
            if (times > 1000) {
                printf("Timeout too many times, exit.\n");
                break;
            }
            printf("Select state(%d) TIMEOUT!\n", times);
            FD_SET(me_fd, &rdfds);
            continue;
        } else {
            printf("State changed inside 1.5s sum ret = %d.\n", ret);
            if (FD_ISSET(0, &rdfds)) {
                flag = 1;
                memset(rply, 0, sizeof(rply));
                rd_stdin = read(0, rply, sizeof(rply));
                printf("Data from usb = %d:\n--- %s.\n", rd_stdin, rply);
            }
        }
        if (flag) {
            flag = 0;
        } else {
            memset(rply, 0, sizeof(rply));
            read(me_fd, rply, sizeof(rply));
            printf("--------\n%s\n--------\n", rply);
        }
        memset(buff, 0, sizeof(buff));
        printf("New serial command: ");
        if (scanf("%64s", rply) == -1) {
            printf("Bytes beyound limit(1~64)!\n");
            break;
        }
        strcpy(buff, rply);
        strcat(buff, "\r\n");
    }
    close(me_fd);
    return times;
}
