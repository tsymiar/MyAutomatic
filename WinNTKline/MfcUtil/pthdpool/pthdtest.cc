#include "pthdpool.h"
#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#define SLEEP while (1) { Sleep(1); }
#else
#define SLEEP ;
#endif
volatile int val = 0;

void *pthd_routine1(void *arg)
{
    int * th = (int*)arg;
    while (val < 10)
    {
        printf("val1: %d [thrd1=%d]\n", val, *th);
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
        printf("val2: %d [thrd2=%d]\n", val, *th);
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
        printf("val3: %d [thrd3=%d]\n", val, *th);
        val = val>>2;
    }
    return NULL;
}

int main()
{
    int thrd = 3;
    pthd_pool_t *pool = pthd_pool_init(5);
    int ret = pthd_pool_add_task(pool, &pthd_routine1, (void*)&thrd);
    printf("task1 ret = %d\n", ret);
    thrd++;
    pthd_pool_wait();
    ret = pthd_pool_add_task(pool, &pthd_routine2, (void*)&thrd);
    printf("task2 ret = %d\n", ret);
    thrd++;
    pthd_pool_wait();
    ret = pthd_pool_add_task(pool, &pthd_routine3, (void*)&thrd);
    printf("task3 ret = %d\n", ret);
    SLEEP
    pthd_pool_destroy(pool);
}
