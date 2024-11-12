#include "mappers.h"

#include <math.h>

double map_sqrt_exp(double number)
{
    return exp(sqrt(number));
}

double map_abs_ctg(double number)
{
    return fabs(1.0 / tan(number));
}

double map_coth_sqrt(double number)
{
    return 1.0 / tanh(sqrt(number));
}


int just_map_matrix(struct matrix* matp, converter fn)
{
    int ret = 0;
    if (!matp) return -1;
    for (size_t i = 0; i < matp->rows; ++i)
        for (size_t j = 0; j < matp->cols; ++j) {
            double value = 0.0;
            ret = double_matrix_get(matp, i, j, &value);
            if (ret) return ret;
            ret = double_matrix_set(matp, i, j, fn(value));
            if (ret) return ret;
        }
    return 0;
}

double map_abs_sin_sum(double a, double b)
{
    return fabs(sin(a + b));
}

int just_map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                      applicator fn)
{
    int ret = 0;
    if (!srcp || !dstp) return -1;
    if (srcp->rows != dstp->rows || srcp->cols != dstp->cols)
        return -1;

    for (size_t i = 0; i < dstp->rows; ++i)
        for (size_t j = 0; j < dstp->cols; ++j) {
            double a = 0.0;
            double b = 0.0;
            ret = double_matrix_get(srcp, i, j, &a);
            if (ret) return ret;
            ret = double_matrix_get(dstp, i, j, &b);
            if (ret) return ret;
            ret = double_matrix_set(dstp, i, j, fn(b, a));
            if (ret) return ret;
        }
    return ret;
}

#ifdef _PTHREAD_H
struct map_args
{
    size_t size;
    double* array;
    converter fn;
};

static void* map_subarray(void* arg)
{
    struct map_args* args = (struct map_args*) arg;
    for (size_t i = 0; i < args->size; ++i)
        args->array[i] = args->fn(args->array[i]);
    return 0;
}

int parallel_map_array(size_t size, double array[size], mapper fn,
                       size_t threads)
{
    int ret = 0;
    pthread_t threads_id[threads];
    struct map_args threads_args[threads];
    const size_t subsize = size / threads;
    const size_t rem = size % threads;
    for (size_t i = 0; i < threads; ++i) {
        threads_args[i].fn = fn;
        threads_args[i].size = subsize + ((i < rem) ? 1 : 0);
        threads_args[i].array = (i > 0)
            ? (threads_args[i - 1].array + threads_args[i - 1].size)
            : array;
        ret = pthread_create(&threads_id[i], NULL, map_subarray,
                       (void*)&threads_args[i]);
        if (ret) return ret;
    }

    for (size_t i = 0; i < threads; ++i) {
        int thread_ret = 0;
        ret = pthread_join(threads_id[i], (void**)&thread_ret);
        if (ret) break;
        ret = thread_ret;
        if (ret) break;
    }

    return ret;
}
#endif
