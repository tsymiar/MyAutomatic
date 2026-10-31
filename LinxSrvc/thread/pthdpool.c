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
 */
 // Modified by tsymiar

#include "pthdpool.h"

static void* pthd_thrd_func(void* argument)
{
    pthd_pool_t* pool = (pthd_pool_t*)argument;
    if (pool == null) {
        return (void*)-1;
    }
    /* Lock must be taken to wait on conditional variable */
    pthread_mutex_lock(&pool->queue_lock);
    for (;;) {
        /* Wait for work, looping to absorb spurious wakeups */
        while ((pool->cur_thrd_no == 0) && (!pool->dispose)) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_lock);
        }
        /* Exit only when disposed and the queue is empty, else drain remaining tasks */
        if (pool->cur_thrd_no == 0) {
            break;
        }
        /* Grab our task */
        pthd_task_t task_t = pool->queue[pool->prev];
        pool->queue[pool->prev].func = null;
        pool->queue[pool->prev].arg = null;
        pool->prev += 1;
        pool->prev = (pool->prev >= pool->queue_size) ? 0 : pool->prev;
        pool->cur_thrd_no -= 1;
        pool->busy_thrd_no += 1;
        /* Unlock while running the task */
        pthread_mutex_unlock(&pool->queue_lock);
        /* Get to work */
        (*(task_t.func))(task_t.arg);
        pthread_mutex_lock(&pool->queue_lock);
        pool->busy_thrd_no -= 1;
        /* Queue empty and nothing running: broadcast also wakes pthd_pool_wait */
        pthread_cond_broadcast(&pool->queue_cond);
    }
    pthread_mutex_unlock(&pool->queue_lock);
    pthread_exit(null);
    return 0;
}

int pthd_pool_free(pthd_pool_t* pool) {
    if (pool == null || pool->dispose) {
        return -1;
    }
    /* Free threads, queue, locks, condvars and the pool itself */
    if (pool->stack) {
        pthread_attr_destroy(&pool->attr);
        free(pool->stack);
    }
    if (pool->thrd_id) {
        pthread_mutex_destroy(&pool->queue_lock);
        pthread_cond_destroy(&pool->queue_cond);
        free(pool->thrd_id);
        free(pool->queue);
    }
    free(pool);
    return 0;
}

pthd_pool_t* pthd_pool_init(int sz_que, int thrd_num)
{
    // Allocate the pool struct
    pthd_pool_t* pool = (pthd_pool_t*)calloc(1, sizeof(pthd_pool_t));
    pool->dispose = false;
    if (thrd_num < 1)
        thrd_num = 1;
    pool->thrd_num = thrd_num;
    pool->queue_size = sz_que;
    pool->cur_thrd_no = 0;
    // Allocate the thread ids and the task queue
    int thd_len = sizeof(pthread_t) * thrd_num;
    int que_len = sizeof(pthd_task_t) * sz_que;
    pool->thrd_id = (pthread_t*)malloc(thd_len);
    if (pool->thrd_id != null)
        memset(pool->thrd_id, 0, thd_len);
    pool->queue = (pthd_task_t*)malloc(que_len);
    if (pool->queue != null)
        memset(pool->queue, 0, que_len);
    if (pthread_mutex_init(&pool->queue_lock, null) != 0 ||
        pthread_cond_init(&pool->queue_cond, null) != 0 ||
        pool->thrd_id == null || pool->queue == null) {
        goto failure;
    }
    /* No custom stack: the old code shared one stack across threads, causing corruption */
    int i, j;
    for (i = j = 0; i < pool->thrd_num; i++) {
        if (pthread_create(&(pool->thrd_id[i]), null,
            pthd_thrd_func, (void*)pool) != 0) {
            goto failure;
        } else {
            threads[j] = i;
            j++;
        }
    }
    return pool;
failure:
    pthd_pool_free(pool);
    return null;
}

int pthd_pool_add_task(pthd_pool_t* pool, pthd_func_t func, void* args)
{
    int err = 0;
    if (pool == null || func == null) {
        return -1;
    }
    if (pthread_mutex_lock(&pool->queue_lock) != 0) {
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
        pool->queue[pool->tail].func = func;
        pool->queue[pool->tail].arg = args;
        pool->tail = next;
        pool->cur_thrd_no += 1;
        /* Workers and pthd_pool_wait share this condvar, so only broadcast works */
        if (pthread_cond_broadcast(&(pool->queue_cond)) != 0) {
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
    if (pthread_mutex_lock(&pool->queue_lock) != 0) {
        return -2;
    }
    if (pool->dispose) {
        pthread_mutex_unlock(&pool->queue_lock);
        return -4;
    }
    /* Set the dispose flag before waking, or workers would never exit */
    pool->dispose = true;
    /* Wake up all worker threads */
    if ((pthread_cond_broadcast(&pool->queue_cond) != 0) ||
        (pthread_mutex_unlock(&pool->queue_lock) != 0)) {
        err = -2;
    }
    /* Join all worker thread */
    int i = 0;
    for (; i < pool->thrd_num; i++) {
        /* pthread_t is unsigned on Linux; the old int<0 guard skipped the join */
        if (pthread_join(pool->thrd_id[threads[i]], null) != 0) {
            err = -3;
        }
    }
    /* pthd_pool_free refuses a disposed pool, so clear the flag first */
    pool->dispose = false;
    if (pthd_pool_free(pool) != 0)
        err = -5;
    return err;
}

int pthd_pool_wait(pthd_pool_t* pool)
{
    if (pool == null)
        return -1;
    if (pthread_mutex_lock(&pool->queue_lock) != 0)
        return -2;
    /* Done only when the queue is empty and no task is running */
    while ((pool->cur_thrd_no > 0) || (pool->busy_thrd_no > 0)) {
        pthread_cond_wait(&pool->queue_cond, &pool->queue_lock);
    }
    pthread_mutex_unlock(&pool->queue_lock);
    return 0;
}
