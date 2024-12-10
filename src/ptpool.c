#include "ptpool.h"
#include <stdint.h>
#include <stdlib.h>

// ptpool_task: begin
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
// ptpool_task: end

// ptpool_tqueue: begin
static bool ptpool_tqueue_init(struct ptpool_tqueue* queue, size_t workers_alive)
{
    queue->head = NULL;
    queue->tail = NULL;
    queue->stop = false;
    queue->busy_workers = 0;
    queue->workers_alive = workers_alive;
    if (pthread_mutex_init(&(queue->task_lock), NULL))
        goto err_task_lock;
    if (pthread_cond_init(&(queue->if_new_task), NULL))
        goto err_if_new_task;
    if (pthread_cond_init(&(queue->if_idle), NULL))
        goto err_if_idle;
    return true;

err_if_idle:
    pthread_cond_destroy(&(queue->if_new_task));
err_if_new_task:
    pthread_mutex_destroy(&(queue->task_lock));
err_task_lock:
    return false;
}

static void ptpool_tqueue_clear(struct ptpool_tqueue* queue)
{
    struct ptpool_task* it = NULL;
    if (!queue) return;

    pthread_mutex_lock(&(queue->task_lock));
    it = queue->head;
    while (it) {
        struct ptpool_task* next = it->next;
        ptpool_task_destroy(it);
        it = next;
    }
    queue->head = NULL;
    queue->stop = true;
    pthread_cond_broadcast(&(queue->if_new_task));
    pthread_mutex_unlock(&(queue->task_lock));
}

static void ptpool_tqueue_free(struct ptpool_tqueue* queue)
{
    pthread_cond_destroy(&(queue->if_new_task));
    pthread_cond_destroy(&(queue->if_idle));
    pthread_mutex_destroy(&(queue->task_lock));
}

static void ptpool_enqueue_tqueue(struct ptpool_tqueue* queue,
                                  struct ptpool_task* task)
{
    pthread_mutex_lock(&(queue->task_lock));
    if (!queue->head) {
        queue->head = task;
        queue->tail = queue->head;
    } else {
        queue->tail->next = task;
        queue->tail = task;
    }
    pthread_cond_broadcast(&(queue->if_new_task));
    pthread_mutex_unlock(&(queue->task_lock));
}

static struct ptpool_task* ptpool_dequeue_task(struct ptpool_tqueue* queue)
{
    struct ptpool_task* task = NULL;

    if (!queue) return NULL;
    task = queue->head;
    if (!task) return NULL;

    if (task->next) {
        queue->head = task->next;
        return task;
    }
    queue->head = NULL;
    queue->tail = NULL;
    return task;
}

static void ptpool_tqueue_wait(struct ptpool_tqueue* queue)
{
    if (!queue) return;

    pthread_mutex_lock(&(queue->task_lock));
    while (1) {
        if (queue->head || (!queue->stop && queue->busy_workers > 0)
            || (queue->stop && queue->workers_alive > 0)) {
            pthread_cond_wait(&(queue->if_idle), &(queue->task_lock));
            continue;
        }
        break;
    }
    pthread_mutex_unlock(&(queue->task_lock));
}

static bool ptpool_tqueues_init(struct ptpool* pool)
{
    pool->queues = malloc(sizeof(*pool->queues) * pool->qlen);
    if (!pool->queues)
        return false;

    for (size_t i = 0; i < pool->qlen; ++i) {
        if (ptpool_tqueue_init(&pool->queues[i], pool->workers / pool->qlen))
            continue;
        for (size_t j = 0; j < i; ++j)
            ptpool_tqueue_free(&(pool->queues[j]));
        free(pool->queues);
        return false;
    }
    return true;
}

static void ptpool_tqueues_clear(struct ptpool* pool)
{
    pthread_mutex_lock(&(pool->pool_lock));
    for (size_t i = 0; i < pool->qlen; ++i)
        ptpool_tqueue_clear(&(pool->queues[i]));
    pthread_mutex_unlock(&(pool->pool_lock));
}

static void ptpool_tqueues_free(struct ptpool* pool)
{
    for (size_t i = 0; i < pool->qlen; ++i)
        ptpool_tqueue_free(&(pool->queues[i]));
}
// ptpool_tqueue: end

static void* ptpool_worker_routine(void* args)
{
    struct ptpool_tqueue* wqueue = (struct ptpool_tqueue*) args;
    struct ptpool_task* task;
    if (!wqueue)
        return (void*) ((intptr_t) -1);

    while (true) {
        pthread_mutex_lock(&(wqueue->task_lock));
        while (!wqueue->head && !wqueue->stop)
            pthread_cond_wait(&(wqueue->if_new_task), &(wqueue->task_lock));

        if (wqueue->stop) break;

        task = ptpool_dequeue_task(wqueue);
        wqueue->busy_workers++;
        pthread_mutex_unlock(&(wqueue->task_lock));

        if (task) {
            task->routine(task->args);
            ptpool_task_destroy(task);
        }

        pthread_mutex_lock(&(wqueue->task_lock));
        wqueue->busy_workers--;
        if (!wqueue->stop && wqueue->busy_workers == 0 && !wqueue->head)
            pthread_cond_signal(&(wqueue->if_idle));
        pthread_mutex_unlock(&(wqueue->task_lock));
    }

    wqueue->workers_alive--;
    pthread_cond_signal(&(wqueue->if_idle));
    pthread_mutex_unlock(&(wqueue->task_lock));
    return (void*) ((intptr_t) 0);
}

// ptpool: begin
static struct ptpool_tqueue* get_worker_tqueue(struct ptpool* pool, size_t wid)
{
    if (!pool) return NULL;
    switch (pool->type) {
    case PTPOOLT_STATIC:
        return &(pool->queues[wid]);
    case PTPOOLT_DYNAMIC:
        return &(pool->queues[0]);
    default:
        return NULL;
    }
}

struct ptpool* ptpool_create(size_t workers, enum ptpool_type type)
{
    struct ptpool* pool = NULL;
    if (0 == workers) return NULL;

    pool = malloc(sizeof(*pool));
    if (!pool) return NULL;

    pthread_t tids[workers];

    pool->type = type;
    pool->workers = workers;
    pool->qid = 0;
    pool->qlen = (pool->type == PTPOOLT_STATIC) ? (workers) : (1);
    if (pthread_mutex_init(&(pool->pool_lock), NULL))
        goto err_pool_lock;
    if (!ptpool_tqueues_init(pool))
        goto err_create_tqueues;

    for (size_t i = 0; i < workers; ++i) {
        if (!pthread_create(&tids[i], NULL, ptpool_worker_routine,
                            get_worker_tqueue(pool, i))) {
            if (!pthread_detach(tids[i]))
                continue;
        }
        for (size_t j = 0; j < i; ++j)
            pthread_cancel(tids[j]);
        goto err_workers;
    }

    return pool;
err_workers:
    ptpool_tqueues_free(pool);
err_create_tqueues:
    pthread_mutex_destroy(&(pool->pool_lock));
err_pool_lock:
    free(pool);
    return NULL;
}

void ptpool_wait(struct ptpool* pool)
{
    if (!pool) return;

    pthread_mutex_lock(&(pool->pool_lock));
    for (size_t i = 0; i < pool->qlen; ++i)
        ptpool_tqueue_wait(&(pool->queues[i]));
    pthread_mutex_unlock(&(pool->pool_lock));
}

void ptpool_destroy(struct ptpool* pool)
{
    if (!pool) return;

    ptpool_tqueues_clear(pool);
    ptpool_wait(pool);

    ptpool_tqueues_free(pool);
    pthread_mutex_destroy(&(pool->pool_lock));
    free(pool);
    pool = NULL;
}

bool ptpool_enqueue_task(struct ptpool* pool, ptpool_routine* routine,
                         void* args)
{
    struct ptpool_task* task = NULL;
    if (!pool) return false;

    task = ptpool_task_create(routine, args);
    if (!task) return false;

    pthread_mutex_lock(&(pool->pool_lock));
    struct ptpool_tqueue* queue = &(pool->queues[pool->qid]);
    ptpool_enqueue_tqueue(queue, task);
    if (pool->type == PTPOOLT_STATIC)
        pool->qid = (pool->qid + 1) % pool->qlen;
    pthread_mutex_unlock(&(pool->pool_lock));

    return true;
}
// ptpool: end
