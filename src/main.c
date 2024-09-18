#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>

#include "mappers.h"
#include "sorts.h"

#ifndef __GNUC__

// here we assume that unsigned is 32bit integer
static unsigned rand_r(unsigned *seed)
{
	(*seed) ^= (*seed) >> 11;
	(*seed) ^= (*seed) << 7 & 0x9D2C5680;
	(*seed) ^= (*seed) << 15 & 0xEFC60000;
	(*seed) ^= (*seed) >> 18;
	return (*seed);
}

#endif

static inline bool is_even(double number)
{
    return !(((uint_least64_t) number) % 2);
}

static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, size_t threads);
static void* select_max_routine(void* arg);
static void* select_map_abs_ctg_routine(void* arg);
static int parallel_selector_arrays(size_t size, double src[size],
                                    double dst[size], void* (*routine)(void* arg),
                                    size_t threads);
static int parallel_reduce_array(double* acc, size_t size, double array[size],
                                 double min, size_t threads);

static int args_parse(int argc, char* argv[], int* Np)
{
    int N = 0, ret = 0;
    if (argc < 2 || !Np) {
        ret = EINVAL;
        goto usage;
    }
    N = strtol(argv[1], NULL, 10);
    ret = errno || (N <= 1);
    if (ret)
        goto usage;

    *Np = N;
    return 0;
usage:
    printf("Usage: %s N\n\nArguments:\n    N    size of matrices, default 2\n",
           argv[0]);
    return ret;
}

int main(int argc, char* argv[])
{
    const double A = 450.0; // А = Ф * И * О
    int N, ret = 0;
    double min, X;
    size_t i, M;
    struct timeval T1, T2;
    long delta_ms;

    ret = args_parse(argc, argv, &N); // N равен первому параметру командной строки
    if (ret) goto exit;
    M = N / 2;

    double* M1 = malloc(N * sizeof(double));
    double* M2 = malloc(M * sizeof(double));
    double* Mt = malloc(M * sizeof(double));
    Mt[0] = 0.0;

    gettimeofday(&T1, NULL); // запомнить текущее время T1
    for (i = 0; i < 100; ++i) { // 100 экспериментов
        // Generate. Заполнить массив исходных данных размером N
        ret = generate_random_uniform_array(N, M1, 1.0, A, 4);
        if (ret) goto freeMs;
        ret = generate_random_uniform_array(M, M2, A, 10.0 * A, 4);
        if (ret) goto freeMs;
        for (size_t j = 0; j < M - 1; ++j)
            Mt[j + 1] = M2[j];

        // Map. Решить поставленную задачу, заполнить массив с результатами
        ret = parallel_map_array(N, M1, map_sqrt_exp, 4);
        if (ret) goto freeMs;
        ret = parallel_selector_arrays(M, Mt, M2, select_map_abs_ctg_routine, 4);
        if (ret) goto freeMs;

        // #pragma omp parallel for default(none) shared(M1, M2, M)
        ret = parallel_selector_arrays(M, M1, M2, select_max_routine, 4);
        if (ret) goto freeMs;

        // Sort. Отсортировать массив с результатами указанным методом
        sort(M, M2);

        min = M2[0];
        // no pragma because: (1) array is already sorted and (2) just looking for first non-zero minimum
        for (size_t j = 1; j < M && min == 0.0; ++j)
            min = M2[j];

        // Reduce. Сумма синусов элементов M2, у которых при делении на минимальное ненулевое целая часть четная
        X = 0.0;
        ret = parallel_reduce_array(&X, M, M2, min, 4);
        if (ret) goto freeMs;

        printf("X = %lf\n", X);
	}
    gettimeofday(&T2, NULL);; // запомнить текущее время T2

    delta_ms = (T2.tv_sec - T1.tv_sec) * 1000
        + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("N=%d. Milliseconds passed: %ld\n", N, delta_ms);

freeMs:
    free(Mt);
    free(M2);
    free(M1);
exit:
    return ret;
}


static inline double random_double_r(double a, double b, unsigned* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

struct _generate_array_args
{
    double a;
    double b;
    size_t size;
    double* array;
};

static void* _generate_random_subarray(void* arg)
{
    struct timespec ts;
    unsigned seed;
    if (clock_gettime(CLOCK_REALTIME, &ts))
        return (void*)((intptr_t) errno);
    seed = ts.tv_sec ^ ts.tv_nsec;

    struct _generate_array_args* args = (struct _generate_array_args*)arg;
    for (size_t i = 0; i < args->size; ++i)
        args->array[i] = random_double_r(args->a, args->b, &seed);
    return 0;
}

static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, size_t threads)
{
    int ret = 0;
    pthread_t threads_id[threads];
    struct _generate_array_args threads_args[threads];
    const size_t subsize = size / threads;
    const size_t rem = size % threads;
    if (!array) return EINVAL;

    for (size_t i = 0; i < threads; ++i) {
        threads_args[i].a = a;
        threads_args[i].b = b;
        threads_args[i].size = subsize + ((i < rem) ? 1 : 0);
        threads_args[i].array = (i > 0)
            ? (threads_args[i - 1].array + threads_args[i - 1].size)
            : array;
        ret = pthread_create(&threads_id[i], NULL, _generate_random_subarray,
                             (void*) &threads_args[i]);
        if (ret) return ret;
    }

    for (size_t i = 0; i < threads; ++i) {
        int thread_ret = 0;
        ret = pthread_join(threads_id[i], (void**)(&thread_ret));
        if (ret) break;
        ret = thread_ret;
        if (ret) break;
    }

    return ret;
}


struct _selector_args
{
    size_t size;
    double* src;
    double* dst;
};

static void* select_max_routine(void* arg)
{
#define MAX(a, b) (((a) > (b))? (a) : (b))
    struct _selector_args* args = (struct _selector_args*) arg;
    for (size_t i = 0; i < args->size; ++i)
        args->dst[i] = MAX(args->src[i], args->dst[i]);
    return 0;
#undef MAX
}

static void* select_map_abs_ctg_routine(void* arg)
{
    struct _selector_args* args = (struct _selector_args*) arg;
    for (size_t i = 0; i < args->size; ++i)
        args->dst[i] = map_abs_ctg(args->src[i] + args->dst[i]);
    return 0;
}

static int parallel_selector_arrays(size_t size, double src[size],
                                    double dst[size], void* (*routine)(void* arg),
                                    size_t threads)
{
    int ret = 0;
    pthread_t threads_id[threads];
    struct _selector_args threads_args[threads];
    const size_t subsize = size / threads;
    const size_t rem = size % threads;
    for (size_t i = 0; i < threads; ++i) {
        threads_args[i].size = subsize + ((i < rem) ? 1 : 0);
        threads_args[i].src = (i > 0)
            ? (threads_args[i - 1].src + threads_args[i - 1].size)
            : src;
        threads_args[i].dst = (i > 0)
            ? (threads_args[i - 1].dst + threads_args[i - 1].size)
            : dst;
        ret = pthread_create(&threads_id[i], NULL, routine,
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


struct _reduce_args
{
    size_t size;
    double* array;
    double min;
};

static void* add_sin_if_even(void* arg)
{
    double* acc = malloc(sizeof(double));
    if (!acc) return NULL;
    *acc = 0.0;
    struct _reduce_args* args = (struct _reduce_args*) arg;
    for (size_t i = 0; i < args->size; ++i)
        *acc += is_even(args->array[i] / args->min) ? sin(args->array[i]) : 0.0;
    return acc;
}

static int parallel_reduce_array(double* acc, size_t size, double array[size],
                                 double min, size_t threads)
{
    int ret = 0;
    pthread_t threads_id[threads];
    struct _reduce_args threads_args[threads];
    const size_t subsize = size / threads;
    const size_t rem = size % threads;
    for (size_t i = 0; i < threads; ++i) {
        threads_args[i].size = subsize + ((i < rem) ? 1 : 0);
        threads_args[i].array = (i > 0)
            ? (threads_args[i - 1].array + threads_args[i - 1].size)
            : array;
        ret = pthread_create(&threads_id[i], NULL, add_sin_if_even,
                       (void*)&threads_args[i]);
        if (ret) return ret;
    }

    for (size_t i = 0; i < threads; ++i) {
        double* thread_ret = NULL;
        ret = pthread_join(threads_id[i], (void**)&thread_ret);
        if (ret) break;
        if (!thread_ret) break;
        *acc += *thread_ret;
        free(thread_ret);
    }

    return ret;
}
