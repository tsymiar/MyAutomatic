/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */ // Modified by tsymiar

#include "pthdpool.h"

static void* pthd_thrd_func(void* argument)
{
    pthd_task_t task_t;
    pthd_pool_t* pool = (pthd_pool_t*)argument;
    if (pool == null) {
        return (void*)-1;
    }
    /* Lock must be token to wait on conditional variable */
    pthread_mutex_lock(&pool->queue_lock);
    /* Wait on condition variable, check for spurious wakeups.
       When returning from pthread_cond_wait(), we own the lock. */
    while ((pool->cur_thrd_no == 0) && (!pool->dispose)) {
        pthread_cond_wait(&(pool->queue_cond), &(pool->queue_lock));
    }
    if (pool->cur_thrd_no == 0) {
        if (pthread_mutex_trylock(&pool->queue_lock) == EBUSY)
            pthread_mutex_unlock(&pool->queue_lock);
        return (void*)-1;//break;
    }
    /* Grab our task */
    if (!pool->queue[pool->prev].func) {
        pthread_mutex_unlock(&pool->queue_lock);
        return (void*)-2;//break;
    }
    task_t.func = pool->queue[pool->prev].func;
    task_t.arg = pool->queue[pool->prev].arg;
    pool->prev += 1;
    pool->prev = (pool->prev >= pool->queue_size) ? 0 : pool->prev;
    pool->cur_thrd_no -= 1;
    /* Unlock */
    if ((!pool->queue[pool->prev].func) && (pthread_mutex_trylock(&pool->queue_lock) == EBUSY))
        pthread_mutex_unlock(&pool->queue_lock);
    /* Get to work */
    (*(task_t.func))(task_t.arg);
    pthread_mutex_unlock(&pool->queue_lock);
    pthread_exit(null);
    return 0;
}

int pthd_pool_free(pthd_pool_t* pool) {
    if (pool == null || pool->dispose) {
        return -1;
    }
    /* 释放线程 任务队列 互斥锁 条件变量 线程池所占内存资源 */
    if (pool->thrd_id) {
        free(pool->thrd_id);
        free(pool->queue);
        pthread_mutex_destroy(&(pool->queue_lock));
        pthread_cond_destroy(&(pool->queue_cond));
    }
    free(pool);
    return 0;
}

pthd_pool_t* pthd_pool_init(int sz_que, int thrd_num)
{
    //从动态内存存储区中分配1个pthd_pool_t单位长度连续空间
    pthd_pool_t* pool = (pthd_pool_t*)calloc(1, sizeof(*pool));
    pool->dispose = false;
    if (thrd_num < 1)
        thrd_num = 1;
    pool->thrd_num = thrd_num;
    pool->queue_size = sz_que;
    pool->cur_thrd_no = 0;
    //为thrd_id分配thrd_num个pthread_t空间
    int thdlen = sizeof(pthread_t) * thrd_num;
    int quelen = sizeof(pthd_task_t) * sz_que;
    pool->thrd_id = (pthread_t*)malloc(thdlen);
    if (pool->thrd_id != null)
        memset(pool->thrd_id, 0, thdlen);
    pool->queue = (pthd_task_t*)malloc(quelen);
    if (pool->queue != null)
        memset(pool->queue, 0, quelen);
    if (pthread_mutex_init(&pool->queue_lock, null) != 0 ||
        pthread_cond_init(&pool->queue_cond, null) != 0 ||
        pool->thrd_id == null || pool->queue == null) {
        goto error;
    }
    int j;
    for (int i = j = 0; i < pool->thrd_num; i++) {
        if (pthread_create(&(pool->thrd_id[i]), null,
            pthd_thrd_func, (void*)pool) != 0) {
            pthd_pool_destroy(pool);
            return null;
        } else {
            threads[j] = i;
            pool->cur_thrd_no += 1;
            j++;
        }
    }
    return pool;
error:
    if (pool) {
        pthd_pool_free(pool);
    }
    return null;
}

int pthd_pool_add_task(pthd_pool_t* pool, void* (*routine)(void*), void* arg)
{
    int err = 0;
    if (pool == null || routine == null) {
        return -1;
    }
    if (pthread_mutex_lock(&(pool->queue_lock)) != 0) {
        return -2;
    }
    int next = pool->tail + 1;
    next = (next >= pool->queue_size) ? 0 : next;
    do {
        /* Are we full ? */
        if (pool->cur_thrd_no >= pool->queue_size) {
            err = -3;
            break;
        }
        /* Are we shutting down ? */
        if (pool->dispose) {
            err = -4;
            break;
        }
        /* Add task to queue */
        pool->queue[pool->tail].func = routine;
        pool->queue[pool->tail].arg = arg;
        pool->tail = next;
        pool->cur_thrd_no += 1;
        /* pthread_cond_broadcast */
        if (pthread_cond_signal(&(pool->queue_cond)) != 0) {
            err = -2;
            break;
        }
    } while (0);
    if (pthread_mutex_unlock(&pool->queue_lock) != 0) {
        err = -2;
    }
    return err;
}

int pthd_pool_destroy(pthd_pool_t* pool)
{
    int err = 0;
    if (pool == null) {
        return -1;
    }
    if (pthread_mutex_lock(&(pool->queue_lock)) != 0) {
        return -2;
    }
    do {
        if (pool->dispose) {
            err = -4;
            break;
        }
        pool->dispose = true;
        /* Wake up all worker threads */
        if ((pthread_cond_broadcast(&(pool->queue_cond)) != 0) ||
            (pthread_mutex_unlock(&(pool->queue_lock)) != 0)) {
            err = -2;
            break;
        }
        /* Join all worker thread */
        for (int i = 0; i < pool->thrd_num; i++) {
            pthread_t thrd = pool->thrd_id[threads[i]];
            if (pthread_join(thrd, null) != 0) {
                err = -3;
            }
        }
        /* do{...} while(0) */
    } while (0);
    /* Only if everything went well do we deallocate the pool */
    if (!err) {
        pthd_pool_free(pool);
    }
    return err;
}

int pthd_pool_wait(pthd_pool_t& pool)
{
    return (pthread_join(*pool.thrd_id, null) != 0) ? -3 : 0;
}
