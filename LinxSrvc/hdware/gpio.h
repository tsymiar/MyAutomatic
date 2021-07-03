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
    snprintf(value, sizeof(value), GPIO_FILES "/gpio%u/value", gpio);
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

int gpio_set_export(unsigned gpio, int direction_may_change)
{
    int file;
    char pin[64];
    int len = 0;
    ssize_t ret = 0;
    if (direction_may_change == 1) {
        file = open(GPIO_FILES "export", O_WRONLY);
    } else {
        file = open(GPIO_FILES "unexport", O_WRONLY);
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

int gpio_export(unsigned gpio)
{
    return gpio_set_export(gpio, 1);
}

int gpio_unexport(unsigned gpio)
{
    return gpio_set_export(gpio, 0);
}

int gpio_set_direct(unsigned gpio, int direct)
{
    char dir[64];
    int file;
    int ret = 0;
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

int gpio_set_value(unsigned int gpio, int value)
{
    int file;
    char val[64];
    snprintf(val, sizeof(val), GPIO_FILES "gpio%u/value", gpio);
    file = open(val, O_WRONLY);
    if (file < 0) {
        perror("gpio_set_value");
        return file;
    }
    if (value == LOW)
        write(file, "0", 2);
    else
        write(file, "1", 2);
    close(file);
    return 0;
}

int gpio_get_value(unsigned gpio)
{
    int file;
    char val[64];
    char ch;
    int value;
    ssize_t len;
    snprintf(val, sizeof(val), GPIO_FILES "/gpio%u/value", gpio);
    file = open(val, O_RDONLY);
    if (file < 0) {
        perror("gpio_get_value");
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

int gpio_direction_input(unsigned gpio, int value)
{
    int ret = gpio_set_direct(gpio, 0);
    ret += gpio_set_value(gpio, value);
    return ret;
}

int gpio_direction_output(unsigned gpio, int value)
{
    int ret = gpio_set_direct(gpio, 1);
    ret += gpio_set_value(gpio, value);
    return ret;
}
