#include <stdio.h>
#include "pthdpool.h"

volatile int val = 0;

void *pthd_routine1(void *arg)
{
    int * th = (int*)arg;
    while (val < 10)
    {
        printf("thrd1: %d, [val=%d]\n", *th, val);
        val++;
    }
    return NULL;
}

void *pthd_routine2(void *arg)
{
    val = 10;
    int * th = (int*)arg;
    while (val > 0)
    {
        printf("thrd2: %d, [val=%d]\n", *th, val);
        val--;
    }
    return NULL;
}

void *pthd_routine3(void *arg)
{
    val = 0x11111;
    int * th = (int*)arg;
    while (val > 0)
    {
        printf("thrd3: %d, [val=%d]\n", *th, val);
        val = val>>2;
    }
    return NULL;
}

int main()
{
    int thrd = 1;
    pthd_pool_t *pool1 = pthd_pool_init(5);
    int ret = pthd_pool_add_task(pool1, &pthd_routine1, (void*)&thrd);
    printf("task1 ret = %d\n", ret);
    pthd_pool_wait(*pool1);
    thrd++;
	pthd_pool_t *pool2 = pthd_pool_init(3);
    ret = pthd_pool_add_task(pool2, &pthd_routine2, (void*)&thrd);
    printf("task2 ret = %d\n", ret);
    thrd++;
	pthd_pool_t *pool3 = pthd_pool_init(8);
    ret = pthd_pool_add_task(pool3, &pthd_routine3, (void*)&thrd);
    printf("task3 ret = %d\n", ret);
    thrd++;
    ret = pthd_pool_add_task(pool3, &pthd_routine2, (void*)&thrd);
    printf("task4 ret = %d\n", ret);
    thrd++;
    ret = pthd_pool_add_task(pool3, &pthd_routine1, (void*)&thrd);
    printf("task5 ret = %d\n", ret);
    thrd++;
    ret = pthd_pool_add_task(pool3, &pthd_routine3, (void*)&thrd);
    printf("task6 ret = %d\n", ret);
    pthd_pool_wait(*pool3);
    pthd_pool_destroy(pool1);
    pthd_pool_destroy(pool2);
    pthd_pool_destroy(pool3);
}
