#ifndef _PTHDPOOL_H_
#define _PTHDPOOL_H_
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
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
#else
#define bool char
#define true 1
#define false 0
#endif
//线程任务属性
typedef struct Task {
  void* (*func)(void*); /* 任务回调函数指针 */
  void* arg;            /* 传入任务函数的参数 */
} pthd_task_t;
//线程池属性
typedef struct Pool {
  bool               dispose;        /* 标志线程池是否销毁 */
  int                thrd_num;       /* 最大线程数 */
  int                queue_size;     /* 线程队列长度 */
  int                cur_thrd_no;    /* 线程池中活动线程数 */
  int                prev;           /* 队首编号 */
  int                tail;           /* 队尾编号 */
  pthread_t* thrd_id;            /* 线程ID数组首指针 */
  pthd_task_t* queue;            /* 线程队列指针 */
  pthread_mutex_t    queue_lock  /* 线程队列锁 */
#if __cplusplus > 199711L
    = PTHREAD_MUTEX_INITIALIZER
#endif
    ;
  pthread_cond_t     queue_cond  /* 线程同步条件变量 */
#if __cplusplus > 199711L
    = PTHREAD_COND_INITIALIZER
#endif
    ;
  void* stack;
  pthread_attr_t attr;
} pthd_pool_t;

typedef void* (*pthd_func_t)(void*);
//线程数组索引
static int threads[MAX_THRD_NUM];
/**
  * @brief     初始化线程池
  * @param     sz_que: 队列长度
  * @param     thrd_num: 最大线程数
  * @retval    thread point, success; NULL, error
  */
pthd_pool_t* pthd_pool_init(int sz_que, int thrd_num
#ifdef __cplusplus
  = 0
#endif
);
/**
  * @brief     向线程池添加任务
  * @param     pool: 添加任务的线程池
  * @param     func: 任务函数指针
  * @param     args: 传递给routine的参数
  * @retval    0, success; -1, param null; -2, mutex error; -3, queue full; -4, dispose
  */
int pthd_pool_add_task(pthd_pool_t* pool, pthd_func_t func, void* args);
/**
  * @brief     销毁线程池
  * @param     pool: 操作对象
  * @retval    0, success; -1, pool null; -2, mutex error; -3, pthread join error; -4, dispose;
  */
int pthd_pool_destroy(pthd_pool_t* pool);
/**
  * @brief     等待线程池执行
  * @param     pool: 操作对象
  * @retval    0, success; -3, pthread join error;
  */
int pthd_pool_wait(pthd_pool_t* pool);

#ifdef __cplusplus
}
#endif
#endif // _PTHDPOOL_H_
