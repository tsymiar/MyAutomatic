#include <stdio.h>
#include "pthdpool.h"

volatile int value = 0;

void* pthd_test1(void* arg)
{
    int* th = (int*)arg;
    while (value < 10) {
        printf("thrd1: %d, [val=%d]\n", *th, value);
        value++;
    }
    return NULL;
}

void* pthd_test2(void* arg)
{
    value = 10;
    int* th = (int*)arg;
    while (value > 0) {
        printf("thrd2: %d, [val=%d]\n", *th, value);
        value--;
    }
    return NULL;
}

void* pthd_test3(void* arg)
{
    value = 0x11111;
    int* th = (int*)arg;
    while (value > 0) {
        printf("thrd3: %d, [val=%d]\n", *th, value);
        value = value >> 2;
    }
    return NULL;
}

int main(int argc, char** argv)
{
    int thrd = 1;
    pthd_pool_t* pool1 = pthd_pool_init(5, 3);
    int stat = pthd_pool_add_task(pool1, &pthd_test1, (void*)&thrd);
    printf("task1: %d\n", stat);
    pthd_pool_wait(pool1);
    thrd++;
    pthd_pool_t* pool2 = pthd_pool_init(3, 3);
    stat = pthd_pool_add_task(pool2, &pthd_test2, (void*)&thrd);
    printf("task2: %d\n", stat);
    thrd++;
    pthd_pool_t* pool3 = pthd_pool_init(8, 3);
    stat = pthd_pool_add_task(pool3, &pthd_test3, (void*)&thrd);
    printf("task3: %d\n", stat);
    thrd++;
    stat = pthd_pool_add_task(pool3, &pthd_test2, (void*)&thrd);
    printf("task4: %d\n", stat);
    thrd++;
    stat = pthd_pool_add_task(pool3, &pthd_test1, (void*)&thrd);
    printf("task5: %d\n", stat);
    thrd++;
    stat = pthd_pool_add_task(pool3, &pthd_test3, (void*)&thrd);
    printf("task6: %d\n", stat);
    stat = pthd_pool_wait(pool3);
    printf("wait3: %d\n", stat);
    stat = pthd_pool_destroy(pool1);
    printf("pool1: %d\n", stat);
    stat = pthd_pool_destroy(pool2);
    printf("pool2: %d\n", stat);
    stat = pthd_pool_destroy(pool3);
    printf("pool3: %d\n", stat);
    return 0;
}
