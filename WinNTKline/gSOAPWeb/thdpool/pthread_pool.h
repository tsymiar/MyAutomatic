#pragma once
#include <pthread.h>

	//线程任务属性
	typedef struct my_pool_work {
		void*               (*func)(void*);			/* 任务回调函数指针 */
		void                *arg;                   /* 传入任务函数的参数 */
		struct my_pool_work   *prev;				/* 链表头指针 */
		struct my_pool_work   *next;				/* 链表尾指针 */
	} my_pool_work_t;
	//线程池属性
	typedef struct my_pool {
		int             dispose;					/* 标志线程池是否销毁 */
		int             max_thr_num;                /* 最大线程数 */
		int				cur_thr_num;				/* 线程池中活动线程数 */ 
		int				idl_thr_num;				/* 线程池中空闲线程数 */  
		pthread_t*		thr_id;                     /* 线程ID数组 */
		my_pool_work_t*	queue_head;                 /* 线程队列头 */
		my_pool_work_t*	queue_next;                 /* 线程队列尾 */
		pthread_mutex_t queue_lock;					/* 线程队列锁 */
		pthread_cond_t  queue_noti;					/* 条件变量 */
	} my_pool_t;

	/*
	 * @brief     初始化线程池
	 * @param     max_thr_num 最大线程数
	 * @retval    成功-0，失败-其他 
	 */
	extern my_pool_t* my_pool_init(int max_thr_num);
	/*
	 * @brief     向线程池添加任务
	 * @param     routine：任务函数指针，arg：传递给routine的参数
	 * @retval    succeed：0，failed：其他
	 */
	extern int my_pool_add_work(void*(*routine)(void*), void *arg);
	/*
	 * @brief     销毁线程池
	 * @param     none
	 * @retval    none
	 */
	extern void my_pool_destroy();

	extern void my_pool_wait(my_pool_t *pool);

	my_pool_t pool_t;

