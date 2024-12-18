#include "mappers.h"

#include <math.h>

double apply_sqrt_exp(double number)
{
    return exp(sqrt(number));
}

double apply_abs_ctg(double number)
{
    return fabs(1.0 / tan(number));
}

double apply_coth_sqrt(double number)
{
    return 1.0 / tanh(sqrt(number));
}

double combine_abs_sin_sum(double a, double b)
{
    return fabs(sin(a + b));
}

#ifdef USE_OPENCL
#include "oclw.h"

extern cl_context ocl_context;
extern cl_command_queue ocl_queue;

cl_kernel apply_coth_sqrt_kern = NULL;


int map_matrix(struct matrix* matp, applicator fn)
{
    int ret = 0;
    cl_mem ocl_mem = NULL;
    cl_kernel ocl_kern = NULL;
    if (!matp) return -1;
    if (!matp->rows || !matp->cols) return 0;

    if (fn == apply_coth_sqrt)
        ocl_kern = apply_coth_sqrt_kern;
    if (!ocl_kern) return -1;

    cl_event wevents[matp->rows];
    cl_event cevent = NULL;
    cl_event revents[matp->rows];
    const size_t row_size = matp->cols * sizeof(double);

    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &ocl_mem,
                             sizeof(double) * matp->rows * matp->cols, NULL);
    if (ret) goto exit;
    ret = oclw_set_kernel_arg(ocl_kern, 0, sizeof(cl_mem), &ocl_mem, "vector");
    if (ret) goto free_memobj;

    // fill memory object with rows
    for (size_t i = 0; i < matp->rows; ++i) {
        size_t off = sizeof(double) * matp->cols * i;
        void* row = (void*)double_matrix_get_row_mut(matp, i);
        if (!row) {
            ret = -1;
            goto free_memobj;
        }
        ret = oclw_async_write_memobj(ocl_queue, ocl_mem, off, row_size, row,
                                      &wevents[i]);
        if (ret) goto free_memobj;
    }

    // run task on memory object after writes
    ret = oclw_async_run_task_after(ocl_queue, ocl_kern, matp->rows, 1,
                                    matp->rows, wevents, &cevent);
    if (ret) goto free_memobj;

    // read results into original matrix after completion
    for (size_t i = 0; i < matp->rows; ++i) {
        size_t off = sizeof(double) * matp->cols * i;
        void* row = double_matrix_get_row_mut(matp, i);
        if (!row) {
            ret = -1;
            goto free_memobj;
        }
        ret = oclw_async_read_memobj_after(ocl_queue, ocl_mem, off, row_size,
                                           row, 1, &cevent, &revents[i]);
        if (ret) goto free_memobj;
    }

    // wait till everything is completed
    ret = oclw_wait_till_completion(matp->rows, revents);
free_memobj:
    ret |= oclw_destroy_memobj(ocl_mem);
exit:
    return ret;
}

int map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                 combiner fn)
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

int shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   size_t shift)
{
    int ret = 0;
    if (!srcp || !dstp) return -1;
    if (srcp->rows != dstp->rows || srcp->cols != dstp->cols)
        return -1;

    for (size_t i = 0; i < srcp->rows; ++i)
        for (size_t j = 0; j < srcp->cols; ++j) {
            double value = 0.0;
            ret = double_matrix_get(srcp, i, j, &value);
            if (ret) return ret;
            ret = double_matrix_set(dstp, i, (j + shift) % dstp->cols, value);
            if (ret) return ret;
        }
    return ret;
}
#elif defined(USE_PTHREAD)
#include <stdint.h>
#include "ptpool.h"
extern struct ptpool* pool;

struct _map_element_args
{
    struct matrix* matp;
    size_t row;
    size_t col;
    applicator fn;
};

static void* _map_element_routine(void* args)
{
    struct _map_element_args* targs = (struct _map_element_args*)args;
    double value = 0.0;
    if (double_matrix_get(targs->matp, targs->row, targs->col, &value))
        return (void*) ((intptr_t) -1);
    if (double_matrix_set(targs->matp, targs->row, targs->col, targs->fn(value)))
        return (void*) ((intptr_t) -1);
    return (void*) ((intptr_t) 0);
}

int map_matrix(struct matrix* matp, applicator fn)
{
    if (!matp) return -1;
    size_t k = 0;
    struct _map_element_args tasks_args[matp->rows * matp->cols];
    for (size_t i = 0; i < matp->rows; ++i)
        for (size_t j = 0; j < matp->cols; ++j) {
            tasks_args[k].matp = matp;
            tasks_args[k].row = i;
            tasks_args[k].col = j;
            tasks_args[k].fn = fn;
            if (!ptpool_enqueue_task(pool, _map_element_routine, &tasks_args[k]))
                return -1;
            k++;
        }

    ptpool_wait(pool);
    return 0;
}

struct _map_matrices_args
{
    struct matrix* srcp;
    struct matrix* dstp;
    size_t row;
    size_t col;
    combiner fn;
};

static void* _map_matrices_routine(void* args)
{
    struct _map_matrices_args* targs = (struct _map_matrices_args*)args;
    double a = 0.0;
    double b = 0.0;
    if (double_matrix_get(targs->srcp, targs->row, targs->col, &a))
        return (void*) ((intptr_t) -1);
    if (double_matrix_get(targs->dstp, targs->row, targs->col, &b))
        return (void*) ((intptr_t) -1);
    if (double_matrix_set(targs->dstp, targs->row, targs->col, targs->fn(b, a)))
        return (void*) ((intptr_t) -1);

    return (void*) ((intptr_t) 0);
}

int map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                 combiner fn)
{
    if (!srcp || !dstp) return -1;
    if (srcp->rows != dstp->rows || srcp->cols != dstp->cols)
        return -1;

    size_t k = 0;
    struct _map_matrices_args tasks_args[dstp->rows * dstp->cols];
    for (size_t i = 0; i < dstp->rows; ++i)
        for (size_t j = 0; j < dstp->cols; ++j) {
            tasks_args[k].srcp = srcp;
            tasks_args[k].dstp = dstp;
            tasks_args[k].row = i;
            tasks_args[k].col = j;
            tasks_args[k].fn = fn;
            if (!ptpool_enqueue_task(pool, _map_matrices_routine, &tasks_args[k]))
                return -1;
            k++;
        }

    ptpool_wait(pool);
    return 0;
}

struct _shift_matrices_args
{
    struct matrix* srcp;
    struct matrix* dstp;
    size_t row;
    size_t col;
    size_t shift;
};

static void* _shift_matrices_routine(void* args)
{
    struct _shift_matrices_args* targs = (struct _shift_matrices_args*)args;
    size_t i = targs->row;
    size_t j = targs->col;
    size_t shift = targs->shift;
    size_t cols = targs->dstp->cols;
    double value = 0.0;
    if (double_matrix_get(targs->srcp, i, j, &value))
        return (void*) ((intptr_t) -1);

    if (double_matrix_set(targs->dstp, i, (j + shift) % cols, value))
        return (void*) ((intptr_t) -1);
    return (void*) ((intptr_t) 0);
}

int shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   size_t shift)
{
    if (!srcp || !dstp) return -1;
    if (srcp->rows != dstp->rows || srcp->cols != dstp->cols)
        return -1;

    size_t k = 0;
    struct _shift_matrices_args tasks_args[srcp->rows * srcp->cols];
    for (size_t i = 0; i < srcp->rows; ++i)
        for (size_t j = 0; j < srcp->cols; ++j) {
            tasks_args[k].srcp = srcp;
            tasks_args[k].dstp = dstp;
            tasks_args[k].row = i;
            tasks_args[k].col = j;
            tasks_args[k].shift = shift;
            if (!ptpool_enqueue_task(pool, _shift_matrices_routine, &tasks_args[k]))
                return -1;
            k++;
        }

    ptpool_wait(pool);
    return 0;
}
#elif defined(_OPENMP)
int map_matrix(struct matrix* matp, applicator fn)
{
    int ret = 0;
    if (!matp) return -1;
    #pragma omp parallel for collapse(2) default(none) shared(matp, ret, fn) schedule(runtime)
    for (size_t i = 0; i < matp->rows; ++i)
        for (size_t j = 0; j < matp->cols; ++j) {
            double value = 0.0;
            if (ret) continue;
            if (double_matrix_get(matp, i, j, &value)) {
                #pragma omp critical
                {
                    ret = -1;
                }
                continue;
            }
            if (double_matrix_set(matp, i, j, fn(value))) {
                #pragma omp critical
                {
                    ret = -1;
                }
                continue;
            }
        }
    return ret;
}

int map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                 combiner fn)
{
    int ret = 0;
    if (!srcp || !dstp) return -1;
    if (srcp->rows != dstp->rows || srcp->cols != dstp->cols)
        return -1;

    #pragma omp parallel for collapse(2) default(none) shared(srcp, dstp, ret, fn) schedule(runtime)
    for (size_t i = 0; i < dstp->rows; ++i)
        for (size_t j = 0; j < dstp->cols; ++j) {
            double a = 0.0;
            double b = 0.0;
            if (ret) continue;
            if (double_matrix_get(srcp, i, j, &a)) {
                #pragma omp critical
                {
                    ret = -1;
                }
                continue;
            }
            if (double_matrix_get(dstp, i, j, &b)) {
                #pragma omp critical
                {
                    ret = -1;
                }
                continue;
            }

            if (double_matrix_set(dstp, i, j, fn(b, a))) {
                #pragma omp critical
                {
                    ret = -1;
                }
                continue;
            }
        }
    return ret;
}

int shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   size_t shift)
{
    int ret = 0;
    if (!srcp || !dstp) return -1;
    if (srcp->rows != dstp->rows || srcp->cols != dstp->cols)
        return -1;

    #pragma omp parallel for collapse(2) default(none) shared(srcp, dstp, shift, ret) schedule(runtime)
    for (size_t i = 0; i < srcp->rows; ++i)
        for (size_t j = 0; j < srcp->cols; ++j) {
            double value = 0.0;
            if (ret) continue;
            if (double_matrix_get(srcp, i, j, &value)) {
                #pragma omp critical
                {
                    ret = -1;
                }
                continue;
            }

            if (double_matrix_set(dstp, i, (j + shift) % dstp->cols, value)) {
                #pragma omp critical
                {
                    ret = -1;
                }
                continue;
            }
        }
    return ret;
}
#else
int map_matrix(struct matrix* matp, applicator fn)
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
    return ret;
}

int map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                 combiner fn)
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

int shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   size_t shift)
{
    int ret = 0;
    if (!srcp || !dstp) return -1;
    if (srcp->rows != dstp->rows || srcp->cols != dstp->cols)
        return -1;

    for (size_t i = 0; i < srcp->rows; ++i)
        for (size_t j = 0; j < srcp->cols; ++j) {
            double value = 0.0;
            ret = double_matrix_get(srcp, i, j, &value);
            if (ret) return ret;
            ret = double_matrix_set(dstp, i, (j + shift) % dstp->cols, value);
            if (ret) return ret;
        }
    return ret;
}
#endif
