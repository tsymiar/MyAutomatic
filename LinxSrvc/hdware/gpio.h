#pragma once
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#define GPIO_FILES "/sys/class/gpio/"

enum GpioValue {
    LOW = 0,
    HIGH
};

struct GpioChip {
    unsigned long gpio;
    char* name;
};

static struct GpioChip chips[] = {
[0] = {
.gpio = 57,
.name = "CAM1_GPIO"
},
};

int gpio_is_valid(unsigned gpio)
{
    char file[32];
    sprintf(file, "%sgpio%u", GPIO_FILES, gpio);
    return access(file, 0);
}

int gpio_request(unsigned int gpio, char* label)
{
    int file;
    char value[64];
    snprintf(value, sizeof(value), GPIO_FILES "gpio%u/value", gpio);
    file = open(value, O_RDONLY | O_NONBLOCK);
    if (file < 0) {
        perror(label);
    }
    return file;
}

void gpio_free(int file)
{
    close(file);
}

int set_gpio_export(unsigned gpio, int direction_may_change)
{
    int file;
    int len = 0;
    char pin[64];
    ssize_t ret = 0;
    fprintf(stdout, "%sing GPIO %u.\n", direction_may_change == 1 ? "export" : "unexport", gpio);
    if (direction_may_change == 1) {
        file = open(GPIO_FILES "export", O_WRONLY);
    } else {
        file = open(GPIO_FILES "unexport", O_WRONLY);
    }
    if (file < 0) {
        perror(__FUNCTION__);
        return file;
    }
    len = snprintf(pin, sizeof(pin), "%u", gpio);
    ret = write(file, pin, len);
    if (ret == -1) {
        close(file);
        return ret;
    }
    close(file);
    return 0;
}

int set_gpio_direct(unsigned gpio, int direct)
{
    int file;
    int ret = 0;
    char dir[64];
    sprintf(dir, "%sgpio%u/direction", GPIO_FILES, gpio);
    file = open(dir, O_WRONLY);
    if (direct == 0) {
        ret = write(file, "in", 3);
    } else {
        ret = write(file, "out", 4);
    }
    if (ret == -1) {
        close(file);
        return ret;
    }
    close(file);
    return 0;
}

int set_gpio_value(unsigned int gpio, int value)
{
    int file;
    char val[64];
    snprintf(val, sizeof(val), GPIO_FILES "gpio%u/value", gpio);
    file = open(val, O_WRONLY);
    if (file < 0) {
        perror("set_gpio_value");
        return file;
    }
    if (value == LOW)
        write(file, "0", 2);
    else
        write(file, "1", 2);
    close(file);
    return 0;
}

int get_gpio_value(unsigned gpio)
{
    int file;
    char ch;
    int value;
    ssize_t len;
    char val[64];
    snprintf(val, sizeof(val), GPIO_FILES "gpio%u/value", gpio);
    file = open(val, O_RDONLY);
    if (file < 0) {
        perror(__FUNCTION__);
        return -1;
    }
    len = read(file, &ch, 1);
    if (len != 1) {
        close(file);
        return -2;
    }
    if (ch != '0') {
        value = 1;
    } else {
        value = 0;
    }
    close(file);
    return value;
}

int set_gpio_by_direction(unsigned gpio, int value, int direct)
{
    fprintf(stdout, "setting GPIO %u value to %s as %s pin.\n", gpio, value == 0 ? "LOW" : "HEIGH", direct == 0 ? "in" : "out");
    if (set_gpio_direct(gpio, direct) != 0) {
        perror("write GPIO fail");
    }
    return set_gpio_value(gpio, value);;
}

void gpio_hint(int val)
{
    if (val >= 0) {
        printf("GPIO status [%d]\n", val);
        return;
    }
    fprintf(stdout, "Usage:\n./gpio [id] [value/(un)export] [direct]."
        "\n++ put 1 parameter to get, 2 to export, 3 to set."
        "\n-- id: \tset/get/export GPIO pin index"
        "\n-- value: \tvalue set to the pin, LOW=0/HEIGH=1"
        "\n-- export: \texport(1) a pin or not(0)"
        "\n-- direct: \tset GPIO direction as in-0/out-1\n");
}
