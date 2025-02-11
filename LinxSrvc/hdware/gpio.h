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
    const char* name;
};

static struct GpioChip chips[] = {
[0] = {
.gpio = 57,
.name = "CAM1_GPIO"
},
};

int gpio_is_valid(unsigned gpio)
{
    char file[64];
    snprintf(file, sizeof(file), "%sgpio%u", GPIO_FILES, gpio);
    return access(file, F_OK);
}

int gpio_request(unsigned int gpio, const char* label)
{
    char value[64];
    snprintf(value, sizeof(value), GPIO_FILES "gpio%u/value", gpio);
    int file = open(value, O_RDONLY | O_NONBLOCK);
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
    const char* operate = direction_may_change ? "export" : "unexport";
    fprintf(stdout, "%sing GPIO %u.\n", operate, gpio);

    char node[64];
    snprintf(node, sizeof(node), GPIO_FILES "%s", operate);
    int file = open(node, O_WRONLY);
    if (file < 0) {
        perror(__FUNCTION__);
        return file;
    }

    char pin[16];
    int len = snprintf(pin, sizeof(pin), "%u", gpio);
    ssize_t state = write(file, pin, len);
    close(file);

    return state == -1 ? state : 0;
}

int set_gpio_direct(unsigned gpio, int direct)
{
    char dir[64];
    snprintf(dir, sizeof(dir), "%sgpio%u/direction", GPIO_FILES, gpio);
    int file = open(dir, O_WRONLY);
    if (file < 0) {
        perror(__FUNCTION__);
        return file;
    }

    const char* value = direct ? "out" : "in";
    ssize_t state = write(file, value, direct ? 4 : 3);
    close(file);

    return state == -1 ? state : 0;
}

int set_gpio_value(unsigned int gpio, int value)
{
    char val[64];
    snprintf(val, sizeof(val), GPIO_FILES "gpio%u/value", gpio);
    int file = open(val, O_WRONLY);
    if (file < 0) {
        perror("set_gpio_value");
        return file;
    }

    const char* value = value == LOW ? "0" : "1";
    write(file, value, 2);
    close(file);

    return 0;
}

int get_gpio_value(unsigned gpio)
{
    char val[64];
    snprintf(val, sizeof(val), GPIO_FILES "gpio%u/value", gpio);
    int file = open(val, O_RDONLY);
    if (file < 0) {
        perror(__FUNCTION__);
        return -1;
    }

    char ch;
    ssize_t len = read(file, &ch, 1);
    close(file);

    if (len != 1) {
        return -2;
    }

    return ch == '0' ? 0 : 1;
}

int set_gpio_by_direction(unsigned gpio, int value, int direct)
{
    fprintf(stdout, "setting GPIO %u value to %s as %s pin.\n", gpio, value == LOW ? "LOW" : "HIGH", direct == 0 ? "in" : "out");
    if (set_gpio_direct(gpio, direct) != 0) {
        perror("write GPIO fail");
        return -1;
    }
    return set_gpio_value(gpio, value);
}

void gpio_hint(int val)
{
    if (val >= 0) {
        printf("GPIO status [%d]\n", val);
    } else {
        fprintf(stdout, "Usage:\n./gpio [id] [value/(un)export] [direct]."
            "\n++ put 1 parameter to get, 2 to export, 3 to set."
            "\n-- id: \tset/get/export GPIO pin index"
            "\n-- value: \tvalue set to the pin, LOW=0/HIGH=1"
            "\n-- export: \texport(1) a pin or not(0)"
            "\n-- direct: \tset GPIO direction as in-0/out-1\n");
    }
}
