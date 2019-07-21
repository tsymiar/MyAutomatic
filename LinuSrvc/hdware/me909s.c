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
const char *PortName = "/dev/ttyUSB0";

const char ME909Arguments[8][32] = {
    "AT",
    "AT+CMEE=2",
    "AT+CPIN?",
    "AT+CEREG=2",
    "AT^HCSQ?",
    "AT+COPS?",
    "AT^SYSCFGEX=\"03\",3fffffff,1,2,7fffffffffffffff,,",
    "AT^NDISDUP=1,1,\"cmnet\""
};

int mem_usb_check() {
    FILE* fusb = fopen("/sys/kernel/debug/usb/devices", "r");
    if (fusb == NULL) {
        perror("Fail to read me909s device");
        return -1;
    }
    char* vendor, *prodid;
    const int maxlinesize = 256;
    char* sperline = (char *)malloc(maxlinesize);
    while (!feof(fusb)
        && fgets(sperline, maxlinesize - 1, fusb) != NULL
        && sperline[strlen(sperline) - 1] == '\n') {
        if (sperline[0] == 'P') {
            vendor = strstr(sperline, "Vendor=");
            prodid = strstr(sperline, "ProdID=");
            if (memcmp("12d1", (void*)(vendor + 7), 4) == 0 && 
                memcmp("15c1", (void*)(prodid + 7), 4) == 0)
                return 0;
        }
    }
    free(sperline);
    return -2;
}

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
    char buff[64], rply[MAX_BUF_LEN];
    struct termios options;
    int cmdat = 0;

    if (mem_usb_check() == -2 && errno != 13) {
        printf("Failing to check me909s device!\n");
        return -1;
    }
    me_fd = open(PortName, O_RDWR | O_NOCTTY);
    if (me_fd == -1) {
        perror("Can't stat device file ttyUSB0");
        return -1;
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
        printf("Serial of %s is ready...\n", PortName);
    }

    memset(buff, 0, sizeof(buff));
    strcpy(buff, ME909Arguments[0]);
    printf("Commands example below: \n\tATZ\n\tATQ0 V1 E1 S0 = 0\n\tAT + CGDCONT = 1, \"IP\", \"3gnet\"\n\
Writing first AT Command: %s\n", ME909Arguments[0]);

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
                printf("Recievd too many timeout, process exit!\n");
                break;
            }
            if (times % 100 == 0) {
                printf("Select(%d) TIMEOUT!\n", times);
            }
            FD_SET(me_fd, &rdfds);
            continue;
        } else {
            printf("State changed inside 1.5s, select ret = %d.\n", ret);
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
            usleep(100);
            memset(rply, 0, sizeof(rply));
            if (read(me_fd, rply, sizeof(rply)) > 0) {
                cmdat++; // first command is "AT", ME909Arguments[0]
            }
            printf("--------\n%s\n--------\n", rply);
        }
        memset(buff, 0, sizeof(buff));
        if (cmdat <= 7) {
            printf("Current AT Command %d: %s\n", cmdat, ME909Arguments[cmdat]);
            strcpy(buff, ME909Arguments[cmdat]);
        } else {
            printf("New serial command('q' to quiet): ");
            if (scanf("%64s", rply) == -1) {
                printf("Bytes beyound limit(1~64)!\n");
                break;
            }
            if (memcmp("q", rply, 2) == 0)
                return 0;
            strcpy(buff, rply);
        }
        strcat(buff, "\r\n");
        times = 0;
    }
    close(me_fd);
    return times;
}
