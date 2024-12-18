#include "generators.h"

#include <errno.h>
#include <time.h>


static inline double random_double_r(double a, double b, unsigned int* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

// skip parallelization for OpenCL 'cause of inability to get random numbers
// on all supported platforms
#ifdef USE_PTHREAD
#include <stdint.h>
#include "ptpool.h"
extern struct ptpool* pool;

struct _randomize_vec_args
{
    struct matrix* matp;
    size_t row;
    unsigned int* seedp;
    double a;
    double b;
};

static void* _randomize_vec_routine(void* args)
{
    struct _randomize_vec_args* targs = (struct _randomize_vec_args*)args;
    double a = targs->a;
    double b = targs->b;
    for (size_t j = 0; j < targs->matp->cols; ++j) {
        if (double_matrix_set(targs->matp, targs->row, j, random_double_r(a, b,
                              targs->seedp)))
            return (void*) ((intptr_t) -1);
    }

    return (void*) ((intptr_t) 0);
}

int generate_random_matrix(struct matrix* matp, double a, double b,
                           unsigned int seed)
{
    if (!matp) return -1;
    struct _randomize_vec_args tasks_args[matp->rows];
    unsigned int seeds[pool->workers];
    seeds[0] = seed;
    for (size_t i = 1; i < pool->workers; ++i)
        seeds[i] = time(NULL); // that may be unsafe way


    for (size_t i = 0; i < matp->rows; ++i) {
        tasks_args[i].matp = matp;
        tasks_args[i].row = i;
        tasks_args[i].seedp = &seeds[(i % pool->workers)];
        tasks_args[i].a = a;
        tasks_args[i].b = b;
        if (!ptpool_enqueue_task(pool, _randomize_vec_routine, &tasks_args[i]))
            return -1;
    }

    ptpool_wait(pool);
    return 0;
}
#elif defined(_OPENMP)
#include <omp.h>
int generate_random_matrix(struct matrix* matp, double a, double b,
                           unsigned int seed)
{
    int ret = 0;
    if (!matp) return -1;
    unsigned int* seeds = NULL;

    #pragma omp parallel default(none) shared(matp, seeds, ret, a, b, seed)
    {
        #pragma omp single
        {
            seeds = malloc(sizeof(unsigned int) * omp_get_num_threads());
            if (seeds) {
                seeds[0] = seed;
                for (size_t i = 1; i < omp_get_num_threads(); ++i)
                    seeds[i] = time(NULL); // that may be unsafe way
            } else {
                ret = -1;
            }
        }

        if (ret) {
            #pragma omp cancel parallel
        }

        #pragma omp for
        for (size_t i = 0; i < matp->rows; ++i)
            for (size_t j = 0; j < matp->cols; ++j) {
                int lret = double_matrix_set(matp, i, j, random_double_r(a, b,
                    &seeds[omp_get_thread_num()]));
                if (lret) {
                    #pragma omp critical
                    {
                        ret = lret;
                    }
                    #pragma omp cancel for
                }
            }

        #pragma omp single
        {
            free(seeds);
        }
    }
    return ret;
}
#else
int generate_random_matrix(struct matrix* matp, double a, double b,
                           unsigned int seed)
{
    int ret = 0;
    if (!matp) return -1;
    for (size_t i = 0; i < matp->rows; ++i)
        for (size_t j = 0; j < matp->cols; ++j) {
            ret = double_matrix_set(matp, i, j, random_double_r(a, b, &seed));
            if (ret) return ret;
        }
    return ret;
}
#endif
