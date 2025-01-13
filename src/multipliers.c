#include "multipliers.h"

// #define USE_OPENCL
#ifdef USE_OPENCL
#include "oclw.h"

extern cl_command_queue ocl_queue;
cl_kernel multiply_kern = NULL;


int _multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                       struct matrix* restrict C, cl_mem* restrict A_memp,
                       cl_mem* restrict B_memp, cl_mem* restrict C_memp,
                       cl_event event, cl_event* eventp)
{
    int ret = 0;
    if (!A || !B || !C || A->cols != B->rows || A->rows != C->rows
        || B->cols != C->cols)
        return -1;
    if (!ocl_queue || !A_memp || !B_memp || !C_memp || !event || !eventp
        || !multiply_kern)
        return -1;

    ret = oclw_set_kernel_arg(multiply_kern, 0, sizeof(A->cols), &A->cols,
                              "Acols");
    if (ret) goto exit;
    ret = oclw_set_kernel_arg(multiply_kern, 1, sizeof(cl_mem), A_memp, "A");
    if (ret) goto exit;
    ret = oclw_set_kernel_arg(multiply_kern, 2, sizeof(cl_mem), B_memp, "B");
    if (ret) goto exit;
    ret = oclw_set_kernel_arg(multiply_kern, 3, sizeof(cl_mem), C_memp, "C");
    if (ret) goto exit;

    // run task on memory objects after writes
    const size_t global_work_size[2] = { A->rows, B->cols };
    cl_int cl_ret = clEnqueueNDRangeKernel(ocl_queue, multiply_kern, 2, NULL,
                                           global_work_size, NULL, 1, &event,
                                           eventp);
    if (cl_ret != CL_SUCCESS) {
        oclw_error(cl_ret, "Unable to multiply matrices");
        ret = -1;
    }
exit:
    return ret;
}
#elif defined(USE_PTHREADS)
#include "ptpool.h"
extern struct ptpool* pool;

struct _multiply_matrices_args
{
    struct matrix* A;
    struct matrix* B;
    struct matrix* C;
    size_t row;
    size_t col;
};

static void* _multiply_matrices_routine(void* args)
{
    struct _multiply_matrices_args* targs = (struct _multiply_matrices_args*)args;
    double c = 0.0;
    for (size_t k = 0; k < targs->B->rows; ++k) {
        double a = 0.0;
        double b = 0.0;
        if (double_matrix_get(targs->A, targs->row, k, &a))
            return (void*) ((intptr_t) -1);

        if (double_matrix_get(targs->B, k, targs->col, &b))
            return (void*) ((intptr_t) -1);
        c += a * b;
    }

    if (double_matrix_set(targs->C, targs->row, targs->col, c))
        return (void*) ((intptr_t) -1);
    return (void*) ((intptr_t) 0);
}

int _multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                           struct matrix* restrict C)
{
    if (!A || !B || !C || A->cols != B->rows || A->rows != C->rows
        || B->cols != C->cols)
        return -1;
    size_t k = 0;
    struct _multiply_matrices_args tasks_args[A->rows * B->cols];
    // A[a][b] * B[b][c] = C[a][c]
    for (size_t i = 0; i < A->rows; ++i)
        for (size_t j = 0; j < B->cols; ++j) {
            tasks_args[k].A = A;
            tasks_args[k].B = B;
            tasks_args[k].C = C;
            tasks_args[k].row = i;
            tasks_args[k].col = j;
            if (!ptpool_enqueue_task(pool, _multiply_matrices_routine, &tasks_args[k]))
                return -1;
            k++;
        }

    ptpool_wait(pool);
    return 0;
}
#elif defined(_OPENMP)
int _multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                           struct matrix* restrict C)
{
    int ret = 0;
    if (!A || !B || !C || A->cols != B->rows || A->rows != C->rows
        || B->cols != C->cols)
        return -1;
    // A[a][b] * B[b][c] = C[a][c]
    #pragma omp parallel for collapse(2) default(none) shared(A, B, C, ret) schedule(runtime)
    for (size_t i = 0; i < A->rows; ++i)
        for (size_t j = 0; j < B->cols; ++j) {
            double c = 0.0;
            if (ret) continue;
            for (size_t k = 0; k < B->rows; ++k) {
                double a = 0.0;
                double b = 0.0;
                if (double_matrix_get(A, i, k, &a)) {
                    #pragma omp critical
                    {
                        ret = -1;
                    }
                    break;
                }

                if (double_matrix_get(B, k, j, &b)) {
                    #pragma omp critical
                    {
                        ret = -1;
                    }
                    break;
                }
                c += a * b;
            }
            if (ret) continue;
            if (double_matrix_set(C, i, j, c)) {
                #pragma omp critical
                {
                    ret = -1;
                }
            }
        }
    return ret;
}
#else
int _multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                           struct matrix* restrict C)
{
    int ret = 0;
    if (!A || !B || !C || A->cols != B->rows || A->rows != C->rows
        || B->cols != C->cols)
        return -1;
    // A[a][b] * B[b][c] = C[a][c]
    for (size_t i = 0; i < A->rows; ++i)
        for (size_t j = 0; j < B->cols; ++j) {
            double c = 0.0;
            for (size_t k = 0; k < B->rows; ++k) {
                double a = 0.0;
                double b = 0.0;
                ret = double_matrix_get(A, i, k, &a);
                if (ret) return ret;
                ret = double_matrix_get(B, k, j, &b);
                if (ret) return ret;
                c += a * b;
            }
            ret = double_matrix_set(C, i, j, c);
            if (ret) return ret;
        }
    return ret;
}
#endif
