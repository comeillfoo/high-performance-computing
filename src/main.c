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
#include "futils.h"

#define KERNEL_PATH "./kernel.clbin"

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
static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, unsigned seed);


int main(int argc, char* argv[])
{
    const double A = 450.0; // А = Ф * И * О
    int N, ret = 0;
    double min, X;
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

    cl_kernel map_sqrt_exp_kern = NULL;
    cl_mem M1_memobj = NULL;

    cl_kernel filter_fold_kern = NULL;
    cl_mem X_memobj = NULL;
    cl_mem M2_memobj = NULL;

    cl_kernel map_abs_ctg_kern = NULL;
    cl_mem Mt_memobj = NULL;

    ret = args_parse(argc, argv, &N); // N равен первому параметру командной строки
    if (ret) goto exit;
    M = N / 2;

    kernbin_stream = fopen(KERNEL_PATH, "rb");
    if (!kernbin_stream) {
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

    ret = oclw_get_default_platform(&cl_platform_id);
    if (ret) { free(kernbin_buf); goto exit; }
    ret = oclw_get_default_device(cl_platform_id, &cl_device_id);
    if (ret) { free(kernbin_buf); goto exit; }
    ret = oclw_create_context(&cl_device_id, &cl_context);
    if (ret) { free(kernbin_buf); goto exit; }
    ret = oclw_create_cmd_queue(cl_context, cl_device_id, &cl_queue);
    if (ret) { free(kernbin_buf); goto cl_free_context; }
    ret = oclw_create_program_from_binary(cl_context, cl_device_id, &cl_program,
                                          sizeof(unsigned char) * kernbin_length,
                                          kernbin_buf);
    if (ret) { free(kernbin_buf); goto cl_free_cmd_queue; }
    ret = oclw_build_program(cl_program, cl_device_id, NULL);
    if (ret) { free(kernbin_buf); goto cl_free_program; }

    // init map_sqrt_exp kernel object + memory object
    ret = oclw_create_kernobj_for_function("map_sqrt_exp", cl_program,
                                           &map_sqrt_exp_kern);
    if (ret) { free(kernbin_buf); goto cl_free_program; }
    ret = oclw_create_memobj(cl_context, CL_MEM_READ_WRITE, &M1_memobj, N *
                             sizeof(double), NULL);
    if (ret) { free(kernbin_buf); goto cl_free_map_sqrt_exp_kern; }
    ret = oclw_set_map_sqrt_exp_args(map_sqrt_exp_kern, N, &M1_memobj);
    if (ret) { free(kernbin_buf); goto cl_free_M1_memobj; }

    // init filter_fold kernel object + memory objects
    ret = oclw_create_kernobj_for_function("filter_fold", cl_program,
                                           &filter_fold_kern);
    if (ret) { free(kernbin_buf); goto cl_free_M1_memobj; }
    ret = oclw_create_memobj(cl_context, CL_MEM_WRITE_ONLY, &X_memobj,
                             sizeof(double), NULL);
    if (ret) { free(kernbin_buf); goto cl_free_filter_fold_kern; }
    ret = oclw_create_memobj(cl_context, CL_MEM_READ_ONLY, &M2_memobj, M *
                             sizeof(double), NULL);
    if (ret) { free(kernbin_buf); goto cl_free_X_memobj; }
    ret = oclw_set_filter_fold_args(filter_fold_kern, M, &M2_memobj, &X_memobj);
    if (ret) { free(kernbin_buf); goto cl_free_M2_memobj; }

    // init map_abs_ctg kernel object + memory objects
    ret = oclw_create_kernobj_for_function("map_abs_ctg", cl_program,
                                           &map_abs_ctg_kern);
    if (ret) { free(kernbin_buf); goto cl_free_M2_memobj; }
    ret = oclw_create_memobj(cl_context, CL_MEM_WRITE_ONLY, &Mt_memobj, M *
                             sizeof(double), NULL);
    if (ret) { free(kernbin_buf); goto cl_free_map_abs_ctg_kern; }
    // TODO: set map_abs_ctg args
    free(kernbin_buf);

    double* M1 = malloc(N * sizeof(double));
    double* M2 = malloc(M * sizeof(double));
    double* Mt = malloc(M * sizeof(double));
    Mt[0] = 0.0;

    gettimeofday(&T1, NULL); // запомнить текущее время T1
    for (i = 0; i < 100; ++i) { // 100 экспериментов
        // Generate. Заполнить массив исходных данных размером N
        ret = generate_random_uniform_array(N, M1, 1.0, A, i);
        if (ret) goto freeMs;
        ret = generate_random_uniform_array(M, M2, A, 10.0 * A, i);
        if (ret) goto freeMs;
        for (size_t j = 0; j < M - 1; ++j)
            Mt[j + 1] = M2[j];

        // Map. Решить поставленную задачу, заполнить массив с результатами
        ret = oclw_sync_write_memobj(cl_queue, M1_memobj, sizeof(double) * N, M1);
        if (ret) goto freeMs;
        ret = oclw_sync_run_task(cl_queue, map_sqrt_exp_kern);
        if (ret) goto freeMs;
        ret = oclw_sync_read_memobj(cl_queue, M1_memobj, sizeof(double) * N, M1);
        if (ret) goto freeMs;

        // parallelization point: use map_abs_ctg kernel
        for (size_t j = 0; j < M; ++j)
            M2[j] = map_abs_ctg(M2[j] + Mt[j]);

        // parallelization point
        for (size_t j = 0; j < M; ++j)
            M2[j] = (M1[j] > M2[j]) ? M1[j] : M2[j];

        // Sort. Отсортировать массив с результатами указанным методом
        sort(M, M2);

        min = M2[0];
        for (size_t j = 1; j < M && min == 0.0; ++j)
            min = M2[j];

        // Reduce. Сумма синусов элементов M2, у которых при делении на минимальное ненулевое целая часть четная
        ret = oclw_sync_write_memobj(cl_queue, M2_memobj, sizeof(double) * M, M2);
        if (ret) goto freeMs;
        ret = oclw_set_filter_fold_min(filter_fold_kern, min);
        if (ret) goto freeMs;
        ret = oclw_sync_run_task(cl_queue, filter_fold_kern);
        if (ret) goto freeMs;
        ret = oclw_sync_read_memobj(cl_queue, X_memobj, sizeof(double), &X);
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
    ret |= oclw_destroy_memobj(Mt_memobj);
cl_free_map_abs_ctg_kern:
    ret |= oclw_destroy_kernel_object(map_abs_ctg_kern);
cl_free_M2_memobj:
    ret |= oclw_destroy_memobj(M2_memobj);
cl_free_X_memobj:
    ret |= oclw_destroy_memobj(X_memobj);
cl_free_filter_fold_kern:
    ret |= oclw_destroy_kernel_object(filter_fold_kern);
cl_free_M1_memobj:
    ret |= oclw_destroy_memobj(M1_memobj);
cl_free_map_sqrt_exp_kern:
    ret |= oclw_destroy_kernel_object(map_sqrt_exp_kern);
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

static inline double random_double_r(double a, double b, unsigned* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, unsigned seed)
{
    if (!array) return EINVAL;

    for (size_t i = 0; i < size; ++i)
        array[i] = random_double_r(a, b, &seed);
    return 0;
}
