#ifndef _PTPOOL_H_
#define _PTPOOL_H_

#include <stdbool.h>
#include <pthread.h>

typedef void* (ptpool_routine)(void*);

struct ptpool_task {
    ptpool_routine* routine;
    void* args;
    struct ptpool_task* next;
};

struct ptpool {
    struct ptpool_task* head;   // work_first
    struct ptpool_task* tail;   // work_last
    pthread_mutex_t task_lock;  // work_mutex
    pthread_cond_t if_new_task; // work_cond
    pthread_cond_t if_all_idle; // working_cond
    size_t busy_workers;        // working_cnt
    size_t workers_alive;       // thread_cnt
    bool stop;
    size_t workers;
    pthread_t tids[];           // flexible array member
};

struct ptpool* ptpool_create(size_t workers);
void ptpool_destroy(struct ptpool* pool);
bool ptpool_enqueue_task(struct ptpool* pool, ptpool_routine* routine,
                         void* args);
void ptpool_wait(struct ptpool* pool);

#endif // _PTPOOL_H_
