#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>

#include "oclw.h"
#include "mappers.h"
#include "sorts.h"

#define KERNEL_PATH "./kernel.elf"

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

static int args_parse(int argc, char* argv[], int* Np);
static inline bool is_even(double number);
static ssize_t fget_size_verbose(FILE* stream);
static int fclose_verbose(FILE* stream);
#ifdef _PTHREAD_H
static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, size_t threads);
static int parallel_selector_arrays(size_t size, double src[size],
                                    double dst[size], size_t threads);
static int parallel_reduce_array(double* acc, size_t size, double array[size],
                                 double min, size_t threads);
#else
static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, unsigned seed);
#endif


int main(int argc, char* argv[])
{
    const double A = 450.0; // А = Ф * И * О
    int N, ret = 0;
    double prev, curr, X;
    size_t i, M;
    struct timeval T1, T2;
    long delta_ms;

    FILE* kernbin_stream;
    ssize_t kernbin_length;
    unsigned char* kernbin_buf = NULL;

    cl_platform_id cl_platform_id = NULL;
    cl_device_id cl_device_id = NULL;
    cl_context cl_context = NULL;
    cl_command_queue cl_queue = NULL;
    cl_program cl_program = NULL;
    cl_kernel filter_fold_kern = NULL;

    ret = args_parse(argc, argv, &N); // N равен первому параметру командной строки
    if (ret) goto exit;
    M = N / 2;

    kernbin_stream = fopen(KERNEL_PATH, "rb");
    if (kernbin_stream == NULL) {
        perror("Unable to open kernel at \'"KERNEL_PATH"\'");
        ret = 1;
        goto exit;
    }

    kernbin_length = fget_size_verbose(kernbin_stream);
    if (kernbin_length == -1) goto exit;

    kernbin_buf = malloc(sizeof(unsigned char) * kernbin_length);
    if (kernbin_buf == NULL) {
        fprintf(stderr, "Unable to allocate %zu bytes for buffer\n",
                kernbin_length);
        ret = fclose_verbose(kernbin_stream) | 1;
        goto exit;
    }

    if (fread(kernbin_buf, sizeof(unsigned char), kernbin_length,
              kernbin_stream) < kernbin_length) {
        fprintf(stderr, "Unable to read kernel binary\n");
        free(kernbin_buf);
        ret = fclose_verbose(kernbin_stream) | 1;
        goto exit;
    }
    fclose_verbose(kernbin_stream);

    ret = oclw_get_platform(&cl_platform_id);
    if (ret) goto exit;
    ret = oclw_select_device(cl_platform_id, &cl_device_id);
    if (ret) goto exit;
    char* cl_device_name = oclw_query_device_name(cl_device_id);
    if (cl_device_name) {
        printf("Selected device with the name %s\n", cl_device_name);
        free(cl_device_name);
    }
    ret = oclw_create_context(&cl_device_id, &cl_context);
    if (ret) goto exit;
    ret = oclw_create_cmd_queue(cl_context, cl_device_id, &cl_queue);
    if (ret) goto cl_free_context;
    ret = oclw_create_program_from_binary(cl_context, cl_device_id, &cl_program,
                                          sizeof(unsigned char) * kernbin_length,
                                          kernbin_buf);
    if (ret) goto cl_free_cmd_queue;
    ret = oclw_create_kernobj_for_function("hello", cl_program, &filter_fold_kern);
    if (ret) goto cl_free_program;
    free(kernbin_buf);

    double* M1 = malloc(N * sizeof(double));
    double* M2 = malloc(M * sizeof(double));

    gettimeofday(&T1, NULL); // запомнить текущее время T1
    for (i = 0; i < 100; ++i) { // 100 экспериментов
        // Generate. Заполнить массив исходных данных размером N
#ifdef _PTHREAD_H
        ret = generate_random_uniform_array(N, M1, 1.0, A, 4);
#else
        ret = generate_random_uniform_array(N, M1, 1.0, A, i);
#endif
        if (ret) goto freeMs;
#ifdef _PTHREAD_H
        ret = generate_random_uniform_array(M, M2, A, 10.0 * A, 4);
#else
        ret = generate_random_uniform_array(M, M2, A, 10.0 * A, i);
#endif
        if (ret) goto freeMs;

        // Map. Решить поставленную задачу, заполнить массив с результатами
#ifdef _PTHREAD_H
        ret = parallel_map_array(N, M1, map_sqrt_exp, 4);
        if (ret) goto freeMs;
#else
        for (size_t j = 0; j < N; ++j)
            M1[j] = map_sqrt_exp(M1[j]);
#endif

        prev = M2[0];
        // no pragmas because: of read/write dependencies
        for (size_t j = 1; j < M; ++j) {
            curr = map_abs_ctg(M2[j] + prev);
            prev = M2[j];
            M2[j] = curr;
        }

#ifdef _PTHREAD_H
        ret = parallel_selector_arrays(M, M1, M2, 4);
        if (ret) goto freeMs;
#else
        for (size_t j = 0; j < M; ++j)
            M2[j] = (M1[j] > M2[j]) ? M1[j] : M2[j];
#endif

        // Sort. Отсортировать массив с результатами указанным методом
        sort(M, M2);

        prev = M2[0];
        for (size_t j = 1; j < M && prev == 0.0; ++j)
            prev = M2[j];

        // Reduce. Сумма синусов элементов M2, у которых при делении на минимальное ненулевое целая часть четная
        X = 0.0;
#ifdef _PTHREAD_H
        ret = parallel_reduce_array(&X, M, M2, prev, 4);
        if (ret) goto freeMs;
#else
        for (size_t j = 0; j < M; ++j)
            X += is_even(M2[j] / prev) ? sin(M2[j]) : 0.0;
#endif
        printf("X = %lf\n", X);
	}
    gettimeofday(&T2, NULL);; // запомнить текущее время T2

    delta_ms = (T2.tv_sec - T1.tv_sec) * 1000
        + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("N=%d. Milliseconds passed: %ld\n", N, delta_ms);

freeMs:
    free(M2);
    free(M1);
    ret |= oclw_destroy_kernel_object(filter_fold_kern);
cl_free_program:
    ret |= oclw_destroy_program_object(cl_program);
cl_free_cmd_queue:
    ret |= oclw_destroy_cmd_queue(cl_queue);
cl_free_context:
    ret |= oclw_destroy_context(cl_context);
exit:
    return ret;
}


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

static inline bool is_even(double number)
{
    return !(((uint_least64_t) number) % 2);
}

static ssize_t fget_size_verbose(FILE* stream)
{
    ssize_t ret = -1;
    if (fseek(stream, 0L, SEEK_END) == -1) {
        perror("Unable to seek stream to the end");
        goto error;
    }
    ret = ftell(stream);
    if (ret == -1) {
        perror("Unable to get current position of binary");
        goto error;
    }
    if (fseek(stream, 0L, SEEK_SET) == -1) {
        perror("Unable to rewind stream back to beginning");
        goto error;
    }
    return ret;
error:
    fclose_verbose(stream);
    return -1;
}

static int fclose_verbose(FILE* stream)
{
    if (fclose(stream) == EOF) {
        perror("Unable to close file");
        return 1;
    }
    return 0;
}

static inline double random_double_r(double a, double b, unsigned* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

#ifdef _PTHREAD_H
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

static int parallel_selector_arrays(size_t size, double src[size],
                                    double dst[size], size_t threads)
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
        ret = pthread_create(&threads_id[i], NULL, select_max_routine,
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
#else
static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, unsigned seed)
{
    if (!array) return EINVAL;

    for (size_t i = 0; i < size; ++i)
        array[i] = random_double_r(a, b, &seed);
    return 0;
}
#endif
