#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
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

#define TIME_WAIT 10000
#define MAX_BUF_LEN	1024
const char *PortName = "/dev/ttyUSB0";

const char ME909Arguments[11][49] = {
    "ATZ",
    "AT+CPIN?",
    "AT+COPS?",
    "AT^SYSCFGEX=\"03\",3fffffff,1,2,7fffffffffffffff,,",
    "AT^SYSINFOEX",
    "AT+CGDCONT=16,\"IP\",\"APN\"",
    "AT^AUTHDATA = 16,,,\"usr\",\"card\"",
    "AT^NDISDUP=1,1,\"cmnet\"",
    "AT+CGACT=1,1",
    "AT+CEREG?",
    "ATDT*99***1#"
};

int start_pppd() {
    return 0;
}

int mem_usb_check() {
    FILE* fusb = fopen("/sys/kernel/debug/usb/devices", "r");
    if (fusb == NULL) {
        perror("WARN: fail to read usb list file");
        return -1;
    }
    char* vendor, *prodid;
    const int maxlinesize = 256;
    char* strperline = (char *)malloc(maxlinesize);
    while (!feof(fusb)
        && fgets(strperline, maxlinesize - 1, fusb) != NULL
        && strperline[strlen(strperline) - 1] == '\n') {
        if (strperline[0] == 'P') {
            vendor = strstr(strperline, "Vendor=");
            prodid = strstr(strperline, "ProdID=");
            if (memcmp("12d1", (void*)(vendor + 7), 4) == 0 &&
                memcmp("15c1", (void*)(prodid + 7), 4) == 0)
                free(strperline);
                fclose(fusb);
            return 0;
        }
    }
    free(strperline);
    fclose(fusb);
    return -2;
}

int main(int argc, int **argv)
{
    fd_set rdfds;
    FD_ZERO(&rdfds);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 500;
    char buff[256], rply[MAX_BUF_LEN];
    struct termios options;
    int me_fd, rd_stdin;
    int tries = 0;
    int flag = 0;
    int cmmat = 0;
    int curr = 0;
    int ret;

    if (mem_usb_check() == -2 && errno != 13) {
        printf("Fail to check me909s model!\n");
        return -1;
    }
    me_fd = open(PortName, O_RDWR | O_NOCTTY);
    if (me_fd == -1) {
        perror("Can't open usb file ttyUSB0");
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
        printf("Serial of %s being ready...\n", PortName);
    }

    while (!flag)
    {
        ret = select(STDIN_FILENO + 1, &rdfds, NULL, NULL, &tv);
        if (ret < 0) {
            perror("select");
            break;
        } else if (FD_ISSET(0, &rdfds)) {
            printf("State changed inside 1.5s, select ret = %d.\n", ret);
        } else {
            tries++;
            if (tries == 1 && cmmat == 0) {
                sprintf(buff, "%s\r\n", ME909Arguments[0]);
                write(me_fd, buff, strlen(buff) + 1);
                usleep(TIME_WAIT);
            } else {
                if (tries > 1000) {
                    printf("Cause too many timeout, checking process exit!\n");
                    for (curr = 0; curr < 11; curr++) {
                        printf("\t%s\n", ME909Arguments[curr]);
                    }
                    cmmat = 11;
                    flag = 1;
                } else {
                    flag = 0;
                    continue;
                }
                if (curr < 11) {
                    if (tries % 100 == 0) {
                        printf("Select(%d) TIMEOUT!\n", tries);
                    }
                    FD_SET(me_fd, &rdfds);
                    usleep(TIME_WAIT);
                    continue;
                }
            }
        }
        if (flag) {
            flag = 0;
        } else {
            memset(rply, 0, sizeof(rply));
            if ((rd_stdin = read(me_fd, rply, sizeof(rply))) > 0) {
                if (strstr(rply, ">")) {
                    printf("Enter text to serial: ");
                    memset(buff, 0, sizeof(buff));
                    scanf("%140s", buff);
                    write(me_fd, buff, strlen(buff) + 1);
                    continue;
                } else {
                    if (strstr(rply, "OK") == NULL)
                        continue;
                    if (cmmat == 0)
                        printf("First command '%s' reply:\n--------\n%s\n--------\n", ME909Arguments[0], rply);
                    else
                        printf("Reply from %s:\n--------\n%s\n--------\n", PortName, rply);
                    cmmat++;
                }
            }
            if (strstr(rply, "CONNECT")) {
                printf("--> Don't know what to do!  Starting pppd and hoping for the best.\n");
                start_pppd();
                break;
            }
            usleep(TIME_WAIT);
        }
        memset(buff, 0, sizeof(buff));
        if (cmmat <= 10 && curr == 0) {
            printf("Current AT Command %d: %s\n", cmmat, ME909Arguments[cmmat]);
            strcpy(buff, ME909Arguments[cmmat]);
        } else {
            printf("New serial command('q' to quit): ");
            if (scanf("%64s", rply) == -1) {
                printf("Bytes beyound limit(1~64)!\n");
                break;
            }
            if (memcmp("q", rply, 2) == 0)
                return 0;
            strcpy(buff, rply);
        }
        strcat(buff, "\r\n");
        write(me_fd, buff, strlen(buff) + 1);
        tries = 0;
    }
    close(me_fd);
    return tries;
}
