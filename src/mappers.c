#include "mappers.h"

#include <math.h>


struct map_args
{
    size_t size;
    double* array;
    converter fn;
};

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

void just_map_matrix(size_t rows, size_t cols, double** matrix, converter fn)
{
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            matrix[i][j] = fn(matrix[i][j]);
}

double map_abs_sin_sum(double a, double b)
{
    return fabs(sin(a + b));
}

void just_map_matrices(size_t rows, size_t cols, double** restrict src,
                       double** restrict dst, applicator fn)
{
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            dst[i][j] = fn(dst[i][j], src[i][j]);
}

#ifdef _PTHREAD_H
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
