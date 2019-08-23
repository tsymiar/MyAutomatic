#include "pthdpool.h"
#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#define SLEEP Sleep(1)
#else
#define SLEEP usleep(1000)
#endif
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
    pthd_pool_t *pool = pthd_pool_init(5);
    int ret = pthd_pool_add_task(pool, &pthd_routine1, (void*)&thrd);
    printf("task1 ret = %d\n", ret);
    thrd++;
    pthd_pool_wait(*pool);
    ret = pthd_pool_add_task(pool, &pthd_routine2, (void*)&thrd);
    printf("task2 ret = %d\n", ret);
    thrd++;
    pthd_pool_wait(*pool);
    ret = pthd_pool_add_task(pool, &pthd_routine3, (void*)&thrd);
    printf("task3 ret = %d\n", ret);
    while (1) { 
	    SLEEP; 
	}
    pthd_pool_destroy(pool);
}
