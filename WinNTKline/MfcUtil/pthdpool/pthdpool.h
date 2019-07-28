#pragma once
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cerrno>
#define NEED_FTIME
#include <pthread.h>
#define MAX_THRD_NUM 1024
#if __cplusplus <= 199711L
#define null NULL
#else
#define null nullptr
#endif
#ifdef __cplusplus
/* 对于 C++ 编译器，指定用 C 的语法编译 */
extern "C" {
#endif
    //线程任务属性
    typedef struct PthdTask {
        void*              (*func)(void*); /* 任务回调函数指针 */
        void               *arg;           /* 传入任务函数的参数 */
    } pthd_task_t;
    //线程池属性
    typedef struct PthdPool {
        bool               dispose;        /* 标志线程池是否销毁 */
        int                thrd_num;       /* 最大线程数 */
        int                queue_size;     /* 线程队列长度 */
        int                cur_thrd_no;    /* 线程池中活动线程数 */
        int                prev;           /* 队首编号 */
        int                tail;           /* 队尾编号 */
        pthread_t*         thrd_id;        /* 线程ID数组首指针 */
        pthd_task_t*       queue;          /* 线程队列指针 */
        pthread_mutex_t    queue_lock = PTHREAD_MUTEX_INITIALIZER;  /* 线程队列锁 */
        pthread_cond_t     queue_noti = PTHREAD_COND_INITIALIZER;   /* 线程同步条件变量 */
    } pthd_pool_t;
    //线程数组索引
    static int threads[MAX_THRD_NUM];
    /**
      * @brief     初始化线程池
      * @param     thrd_num: 最大线程数, sz_que: 队列长度
      * @retval    sucess: thread point; NULL, error;
      */
    extern pthd_pool_t* pthd_pool_init(int sz_que, int thrd_num = 0);
    /**
      * @brief     向线程池添加任务
      * @param     pool: 添加任务的线程池; routine: 任务函数指针; arg: 传递给routine的参数
      * @retval    0, sucess; -1, param null; -2, mutex error; -3, queue full; -4, dispose;
      */
    extern int pthd_pool_add_task(pthd_pool_t *pool, void*(*routine)(void*), void *arg);
    /**
      * @brief     等待线程退出
      * @retval    0, sucess; -3, pthread join error;
      */
    extern int pthd_pool_wait();
    /**
      * @brief     销毁线程池
      * @retval    0, sucess; -1, pool null; -2, mutex error; -3, pthread join error; -4, dispose;
      */
    extern int pthd_pool_destroy(pthd_pool_t *pool);

#ifdef __cplusplus
}
#endif
