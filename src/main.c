#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "matrix.h"
#include "generators.h"
#include "mappers.h"
#include "mergers.h"
#include "multipliers.h"
#include "sorts.h"
#include "reducers.h"
#include "futils.h"
#ifdef USE_OPENCL
#include "oclw.h"
#include <stdlib.h>


cl_platform_id ocl_platform_id = NULL;
cl_device_id ocl_device_id = NULL;
cl_context ocl_context = NULL;
cl_command_queue ocl_queue = NULL;
cl_program ocl_program = NULL;

cl_mem M1_mem = NULL;
cl_mem M2_mem = NULL;
cl_mem Mt_mem = NULL;
cl_mem M_mem = NULL;
cl_mem M_psums_mem = NULL;

cl_event map_matrix_event = NULL;
cl_event shift_matrices_event = NULL;
cl_event map_matrices_event = NULL;
cl_event merge_matrices_event = NULL;

// mappers
extern cl_kernel apply_coth_sqrt_kern;
extern cl_kernel combine_abs_sin_sum_kern;
extern cl_kernel shift_matrices_kern;
// mergers
extern cl_kernel merge_by_pow_kern;
// multipliers
extern cl_kernel multiply_kern;
// sorts
extern cl_kernel selection_sort_kern;
// reducers
extern cl_kernel reduce_kern;
#endif

static int args_parse(int argc, char* argv[], int* Np);
static enum matrix_type env_parse_matrix_type();
static int library_init(struct matrix* M1, struct matrix* M2, struct matrix* Mt,
                        struct matrix* M);
static int library_exit();

int main(int argc, char* argv[])
{
    const double A = 400.0; // А = Ф(8) * И(5) * О(10)
    int N, ret = 0;
    enum matrix_type mtype = env_parse_matrix_type();
    struct tstamp T1, T2;
    long delta_ms = 0;

    ret = args_parse(argc, argv, &N);
    if (ret) goto exit;

    struct matrix M1 = {0};
    ret = double_matrix_create(N, N / 2, mtype, &M1);
    if (ret) goto exit;

    struct matrix M2 = {0};
    ret = double_matrix_create(N / 2, N, mtype, &M2);
    if (ret) goto freeM1;

    struct matrix Mt = {0};
    ret = double_matrix_create(N / 2, N, mtype, &Mt);
    if (ret) goto freeM2;

    struct matrix M = {0};
    ret = double_matrix_create(N, N, mtype, &M);
    if (ret) goto freeMt;

    ret = library_init(&M1, &M2, &Mt, &M);
    if (ret) goto freeM;

    ret = stamp_time(&T1);
    if (ret) goto libexit;
    for (size_t i = 0; i < 100; ++i) {
        // Generate. Сформировать матрицу M1[N][N / 2], заполнить uniform(1, A).
        // Сформировать матрицу M2[N / 2][N], заполнить uniform(A, 10.0 * A).
        ret = generate_random_matrix(&M1, 1.0, A, i);
        if (ret) goto libexit;
        ret = generate_random_matrix(&M2, A, 10.0 * A, i);
        if (ret) goto libexit;

        // Map. В матрице M1 к каждому элементу применить операцию из таблицы.
        // В матрице M2 каждую колонку поочередно сложить с предыдущей
        ret = map_matrix(&M1, apply_coth_sqrt);
        if (ret) goto libexit;
        ret = shift_matrices(&M2, &Mt, 1);
        if (ret) goto libexit;
        ret = map_matrices(&Mt, &M2, combine_abs_sin_sum);
        if (ret) goto libexit;

        // Merge. В матрицах М1 и М2 ко всем элементами с одинаковыми индексами
        // попарно применить операцию из таблицы (результат записать в М2).
        ret = merge_matrices(&M1, &M2, merge_by_pow);
        if (ret) goto libexit;

        // Multiply. Умножить матрицы M1 и M2
        // M1[N][N / 2] * M2[N / 2][N] = M[N][N]
        ret = multiply_matrices(&M1, &M2, &M);
        if (ret) goto libexit;

        // Sort. Полученную матрицу M необходимо отсортировать по строкам
        ret = sort_rows(&M);
        if (ret) goto libexit;

        // Reduce. Рассчитать сумму синусов тех элементов матрицы M, которые при
        // делении на минимальный ненулевой элемент массива соответствующей строки дают четное число
        double X = 0.0;
        ret = reduce(&M, &X);
        if (ret) goto libexit;
        printf("X = %lf\n", X);
    }
    ret = stamp_time(&T2);
    if (ret) goto libexit;
    delta_ms = stamps_diff_ms(T1, T2);
    printf("N = %d. Milliseconds passed: %ld\n", N, delta_ms);

libexit:
    if (library_exit()) ret = -1;
freeM:
    double_matrix_destroy(M);
freeMt:
    double_matrix_destroy(Mt);
freeM2:
    double_matrix_destroy(M2);
freeM1:
    double_matrix_destroy(M1);
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
    printf("Usage: %s N\n\nArguments:\n    N    size of matrices, minimum 2\n",
           argv[0]);
    return ret;
}

static enum matrix_type env_parse_matrix_type()
{
    const char* raw_matrix_type = getenv("MATRIX_TYPE");
    if (!raw_matrix_type)
        return MT_TABLE;
    if (!strncmp("vector", raw_matrix_type, 7))
        return MT_VECTOR;
    if (!strncmp("table", raw_matrix_type, 6))
        return MT_TABLE;
    return MT_TABLE;
}

// #define USE_OPENCL
#ifdef USE_OPENCL
#ifdef PARALLEL_SORT_ONLY_ROWS
#define SELECTION_SORT_KERNEL_NAME ("selection_sort_only_row")
#else
#define SELECTION_SORT_KERNEL_NAME ("selection_sort_halves")
#endif


static int library_init(struct matrix* M1, struct matrix* M2, struct matrix* Mt,
                        struct matrix* M)
{
    int ret = 0;
    FILE* kern_bin_stream = NULL;
    ssize_t kern_bin_sz = -1;
    unsigned char* kern_bin = NULL;

    char* kern_bin_path = getenv("OCL_KERNELS_PATH");
    if (!kern_bin_path) {
        kern_bin_path = "./ocl-main.clbin";
    }

    kern_bin_stream = fopen(kern_bin_path, "rb");
    if (!kern_bin_stream) {
        perror("Unable to open kernel");
        ret = 1;
        goto exit;
    }

    kern_bin_sz = fget_size_verbose(kern_bin_stream);
    if (kern_bin_sz == -1) {
        ret = 1;
        goto exit;
    }

    kern_bin = malloc(sizeof(unsigned char) * kern_bin_sz);
    if (!kern_bin) {
        fprintf(stderr, "Unable to allocate %zu bytes for buffer\n",
                kern_bin_sz);
        ret = 1;
        goto free_kern_bin_stream;
    }

    if (fread(kern_bin, sizeof(unsigned char), kern_bin_sz,
              kern_bin_stream) < kern_bin_sz) {
        fprintf(stderr, "Unable to read kernel binary\n");
        ret = 1;
        goto free_kern_bin;
    }

    ret = oclw_get_default_platform(&ocl_platform_id);
    if (ret) goto free_kern_bin;
    ret = oclw_get_default_device(ocl_platform_id, &ocl_device_id);
    if (ret) goto free_kern_bin;
    ret = oclw_create_context(&ocl_device_id, &ocl_context);
    if (ret) goto free_kern_bin;
    ret = oclw_create_cmd_queue(ocl_context, ocl_device_id, &ocl_queue);
    if (ret) goto err_free_ctx;
    ret = oclw_create_program_from_binary(ocl_context, ocl_device_id, &ocl_program,
                                          sizeof(unsigned char) * kern_bin_sz,
                                          kern_bin);
    if (ret) goto err_free_cmd_queue;
    ret = oclw_build_program(ocl_program, ocl_device_id, NULL);
    if (ret) goto err_free_program_obj;

    // init mappers kernels
    ret = oclw_create_kernobj_for_function("apply_coth_sqrt", ocl_program,
                                           &apply_coth_sqrt_kern);
    if (ret) goto err_free_program_obj;
    ret = oclw_create_kernobj_for_function("combine_abs_sin_sum", ocl_program,
                                           &combine_abs_sin_sum_kern);
    if (ret) goto err_free_apply_coth_sqrt;
    ret = oclw_create_kernobj_for_function("shift_matrices", ocl_program,
                                           &shift_matrices_kern);
    if (ret) goto err_free_combine_abs_sin_sum;
    // init mergers kernels
    ret = oclw_create_kernobj_for_function("merge_by_pow", ocl_program,
                                           &merge_by_pow_kern);
    if (ret) goto err_free_shift_matrices;
    // init multipliers kernels
    ret = oclw_create_kernobj_for_function("multiply", ocl_program,
                                           &multiply_kern);
    if (ret) goto err_free_merge_by_pow;
    // init sorts kernels
    ret = oclw_create_kernobj_for_function(SELECTION_SORT_KERNEL_NAME, ocl_program,
                                           &selection_sort_kern);
    if (ret) goto err_free_multiply;
    // init reducers kernels
    ret = oclw_create_kernobj_for_function("reduce", ocl_program, &reduce_kern);
    if (ret) goto err_free_selection_sort;

    // init memory objects
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &M1_mem,
                             sizeof(double) * M1->rows * M1->cols, NULL);
    if (ret) goto err_free_reduce;
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &M2_mem,
                             sizeof(double) * M2->rows * M2->cols, NULL);
    if (ret) goto err_free_M1_mem;
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &Mt_mem,
                             sizeof(double) * Mt->rows * Mt->cols, NULL);
    if (ret) goto err_free_M2_mem;
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &M_mem,
                             sizeof(double) * M->rows * M->cols, NULL);
    if (ret) goto err_free_Mt_mem;
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &M_psums_mem,
                             sizeof(double) * M->rows, NULL);
    if (ret) goto err_free_M_mem;
    if (!ret) goto free_kern_bin;
    // ret |= oclw_destroy_memobj(M_psums_mem);
err_free_M_mem:
    ret |= oclw_destroy_memobj(M_mem);
err_free_Mt_mem:
    ret |= oclw_destroy_memobj(Mt_mem);
err_free_M2_mem:
    ret |= oclw_destroy_memobj(M2_mem);
err_free_M1_mem:
    ret |= oclw_destroy_memobj(M1_mem);
err_free_reduce:
    ret |= oclw_destroy_kernel_object(reduce_kern);
err_free_selection_sort:
    ret |= oclw_destroy_kernel_object(selection_sort_kern);
err_free_multiply:
    ret |= oclw_destroy_kernel_object(multiply_kern);
err_free_merge_by_pow:
    ret |= oclw_destroy_kernel_object(merge_by_pow_kern);
err_free_shift_matrices:
    ret |= oclw_destroy_kernel_object(shift_matrices_kern);
err_free_combine_abs_sin_sum:
    ret |= oclw_destroy_kernel_object(combine_abs_sin_sum_kern);
err_free_apply_coth_sqrt:
    ret |= oclw_destroy_kernel_object(apply_coth_sqrt_kern);
err_free_program_obj:
    ret |= oclw_destroy_program_object(ocl_program);
err_free_cmd_queue:
    ret |= oclw_destroy_cmd_queue(ocl_queue);
err_free_ctx:
    ret |= oclw_destroy_context(ocl_context);
free_kern_bin:
    free(kern_bin);
free_kern_bin_stream:
    ret |= fclose_verbose(kern_bin_stream);
exit:
    return ret;

}

static int library_exit()
{
    int ret = 0;
    ret |= oclw_destroy_memobj(M_psums_mem);
    ret |= oclw_destroy_memobj(M_mem);
    ret |= oclw_destroy_memobj(Mt_mem);
    ret |= oclw_destroy_memobj(M2_mem);
    ret |= oclw_destroy_memobj(M1_mem);
    ret |= oclw_destroy_kernel_object(reduce_kern);
    ret |= oclw_destroy_kernel_object(selection_sort_kern);
    ret |= oclw_destroy_kernel_object(multiply_kern);
    ret |= oclw_destroy_kernel_object(merge_by_pow_kern);
    ret |= oclw_destroy_kernel_object(shift_matrices_kern);
    ret |= oclw_destroy_kernel_object(combine_abs_sin_sum_kern);
    ret |= oclw_destroy_kernel_object(apply_coth_sqrt_kern);
    ret |= oclw_destroy_program_object(ocl_program);
    ret |= oclw_destroy_cmd_queue(ocl_queue);
    ret |= oclw_destroy_context(ocl_context);
    return ret;
}
#elif defined(USE_PTHREAD)
#include "ptpool.h"
#include <stdlib.h>
#include <limits.h>


struct ptpool* pool = NULL;


static int library_init(struct matrix* M1, struct matrix* M2, struct matrix* Mt,
                        struct matrix* M)
{
    (void)M1;
    (void)M2;
    (void)Mt;
    (void)M;
    size_t workers = 2;
    enum ptpool_type type = PTPOOLT_DYNAMIC;
    const char* raw_value = getenv("PT_NUM_THREADS");
    if (raw_value) {
        workers = strtoul(raw_value, NULL, 10);
        workers = (workers == ULONG_MAX)? 2 : workers;
    }

    raw_value = getenv("PT_POOL_TYPE");
    if (raw_value) {
        if (!strncmp("dynamic", raw_value, 8)) {
            type = PTPOOLT_DYNAMIC;
        }
        if (!strncmp("static", raw_value, 7)) {
            type = PTPOOLT_STATIC;
        }
    }

    pool = ptpool_create(workers, type);
    if (!pool)
        return -1;
    return 0;
}

static int library_exit()
{
    ptpool_destroy(pool);
    return 0;
}
#else
static int library_init(struct matrix* M1, struct matrix* M2, struct matrix* Mt,
                        struct matrix* M)
{
    (void)M1;
    (void)M2;
    (void)Mt;
    (void)M;
    return 0;
}

static int library_exit()
{
    return 0;
}
#endif
