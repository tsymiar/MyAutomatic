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

#define MAX_BUFF	1024
#define MAX_GET 	24

void serial_init(int fd)
{
    struct termios options;
    bzero(&options, sizeof(options));

    tcgetattr(fd, &options);
    //位掩码方式激活本地连接、使能接收
    options.c_cflag |= (CLOCAL | CREAD);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag &= ~CSIZE;
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CSTOPB;
    options.c_iflag |= IGNPAR;
    tcflush(fd, TCIFLUSH);

    options.c_oflag = 0;
    options.c_lflag = 0;

    tcflush(fd, TCIOFLUSH);
    if (0 == tcsetattr(fd, TCSANOW, &options)) {
        printf("Serial tty is ready...\n");
    }
}



int main(int argc, int **argv)
{
    fd_set rdfds;
    struct timeval tv;
    int me_fd, ret, rd_stdin;
    int flag = 0;
    char buff[MAX_BUFF], replay[MAX_BUFF];

    me_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (me_fd == -1) {
        perror("Can't open /dev/ttyUSB0 - \n");
        return 0;
    } else {
        fcntl(me_fd, F_SETFL, 0);
    }

    serial_init(me_fd);

    while (!flag)
    {
        FD_ZERO(&rdfds);
        FD_SET(me_fd, &rdfds);
        tv.tv_sec = 1;
        tv.tv_usec = 500;

        memset(buff, 0, sizeof(buff));
        strcpy(buff, "ATI\r\n");
        write(me_fd, buff, sizeof(buff));
        ret = select(me_fd + 1, &rdfds, NULL, NULL, &tv);
        if (ret < 0)
            perror("select");
        else if (ret == 0)
            printf("Select state TIMEOUT!\n");
        else
        {
            printf("State changed inside 1.5s sum ret = %d.\n", ret);
            if (FD_ISSET(0, &rdfds)) {
                flag = 1;
                memset(replay, 0, sizeof(replay));
                rd_stdin = read(0, replay, sizeof(replay));
                printf("Data from stdin = %d:\n--- %s.\n", rd_stdin, replay);
            }
        }
        if (flag) {
            flag = 0;
            memset(buff, 0, sizeof(buff));
            printf("Input serial command: ");
            if (scanf("%64s", replay) == -1) {
                printf("Bytes beyound limit(64)!\n");
            }
            strcpy(buff, replay);
            strcat(buff, "\r\n");
            write(me_fd, buff, sizeof(buff));

            memset(replay, 0, sizeof(replay));
            read(me_fd, replay, sizeof(replay));
            printf("--- %s\n", replay);

        } else {
            memset(replay, 0, sizeof(replay));
            read(me_fd, replay, sizeof(replay));
            printf("--- %s\n", replay);
        }
    }
    close(me_fd);
}
