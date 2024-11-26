#include "ptpool.h"
#include <stdint.h>
#include <stdlib.h>


static struct ptpool* pool = NULL;


static struct ptpool_task* ptpool_task_create(ptpool_routine* routine, void* args)
{
    struct ptpool_task* task = NULL;
    if (!routine) return NULL;

    task = malloc(sizeof(*task));
    if (!task) return NULL;
    task->routine = routine;
    task->args = args;
    task->next = NULL;

    return task;
}

static void ptpool_task_destroy(struct ptpool_task* task)
{
    if (!task) return;
    free(task);
}

static struct ptpool_task* ptpool_dequeue_task(struct ptpool* pool)
{
    struct ptpool_task* task = NULL;

    if (!pool) return NULL;
    task = pool->head;
    if (!task) return NULL;

    if (task->next) {
        pool->head = task->next;
        return task;
    }
    pool->head = NULL;
    pool->tail = NULL;
    return task;
}

static void* ptpool_worker_routine(void* args)
{
    struct ptpool* pool = (struct ptpool*)args;
    struct ptpool_task* task;

    while (true) {
        pthread_mutex_lock(&(pool->task_lock));
        while (!pool->head && !pool->stop)
            pthread_cond_wait(&(pool->if_new_task), &(pool->task_lock));

        if (pool->stop) break;

        task = ptpool_dequeue_task(pool);
        pool->busy_workers++;
        pthread_mutex_unlock(&(pool->task_lock));

        if (task) {
            task->routine(task->args);
            ptpool_task_destroy(task);
        }

        pthread_mutex_lock(&(pool->task_lock));
        pool->busy_workers--;
        if (!pool->stop && pool->busy_workers == 0 && !pool->head)
            pthread_cond_signal(&(pool->if_all_idle));
        pthread_mutex_unlock(&(pool->task_lock));
    }

    pool->workers_alive--;
    pthread_cond_signal(&(pool->if_all_idle));
    pthread_mutex_unlock(&(pool->task_lock));
    return (void*) ((intptr_t) 0);
}


int ptpool_create(size_t workers)
{
    if (0 == workers) return -1;

    pool = malloc(sizeof(*pool));
    if (!pool) return -1;

    pthread_t tids[workers];
    pool->workers_alive = workers;
    pool->busy_workers = 0;
    pool->stop = false;
    pool->head = NULL;
    pool->tail = NULL;

    if (pthread_mutex_init(&(pool->task_lock), NULL))
        goto err_task_lock;

    if (pthread_cond_init(&(pool->if_new_task), NULL))
        goto err_if_new_task;

    if (pthread_cond_init(&(pool->if_all_idle), NULL))
        goto err_if_all_idle;

    for (size_t i = 0; i < workers; ++i) {
        if (!pthread_create(&tids[i], NULL, ptpool_worker_routine, pool)) {
            if (!pthread_detach(tids[i]))
                continue;
        }
        for (size_t j = 0; j < i; ++j)
            pthread_cancel(tids[j]);
        goto err_workers;
    }

    return 0;
err_workers:
    pthread_cond_destroy(&(pool->if_all_idle));
err_if_all_idle:
    pthread_cond_destroy(&(pool->if_new_task));
err_if_new_task:
    pthread_mutex_destroy(&(pool->task_lock));
err_task_lock:
    free(pool);
    pool = NULL;
    return -1;
}

void ptpool_wait()
{
    if (!pool) return;

    pthread_mutex_lock(&(pool->task_lock));
    while (1) {
        if (pool->head || (!pool->stop && pool->busy_workers > 0)
            || (pool->stop && pool->workers_alive > 0)) {
            pthread_cond_wait(&(pool->if_all_idle), &(pool->task_lock));
            continue;
        }
        break;
    }
    pthread_mutex_unlock(&(pool->task_lock));
}

void ptpool_destroy()
{
    struct ptpool_task* it = NULL;
    if (!pool) return;

    pthread_mutex_lock(&(pool->task_lock));
    it = pool->head;
    while (it) {
        struct ptpool_task* next = it->next;
        ptpool_task_destroy(it);
        it = next;
    }

    pool->head = NULL;
    pool->stop = true;
    pthread_cond_broadcast(&(pool->if_new_task));
    pthread_mutex_unlock(&(pool->task_lock));

    ptpool_wait();

    pthread_cond_destroy(&(pool->if_all_idle));
    pthread_cond_destroy(&(pool->if_new_task));
    pthread_mutex_destroy(&(pool->task_lock));
    free(pool);
    pool = NULL;
}

bool ptpool_enqueue_task(ptpool_routine* routine, void* args)
{
    struct ptpool_task* task = NULL;
    if (!pool) return false;

    task = ptpool_task_create(routine, args);
    if (!task) return false;

    pthread_mutex_lock(&(pool->task_lock));
    if (!pool->head) {
        pool->head = task;
        pool->tail = pool->head;
    } else {
        pool->tail->next = task;
        pool->tail = task;
    }
    pthread_cond_broadcast(&(pool->if_new_task));
    pthread_mutex_unlock(&(pool->task_lock));

    return true;
}
