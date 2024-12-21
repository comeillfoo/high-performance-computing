#include "reducers.h"

#include <stdbool.h>
#include <stdint.h>
#include <math.h>


#ifndef USE_OPENCL
static bool is_even_as_integer(double number)
{
    return !(((uint_least64_t) number) % 2);
}
#endif

// #define USE_OPENCL
#ifdef USE_OPENCL
#include "oclw.h"

extern cl_context ocl_context;
extern cl_command_queue ocl_queue;

cl_kernel reduce_kern = NULL;

int reduce(struct matrix* matp, double* reduction)
{
    int ret = 0;
    cl_mem mat_mem = NULL, psums_mem = NULL, mins_mem = NULL;
    if (!matp || !reduction) return -1;
    if (!matp->rows || !matp->cols) return 0;
    if (!ocl_context || !ocl_queue || !reduce_kern) return -1;

    double mins[matp->rows];
    double psums[matp->rows];
    cl_event wevents[matp->rows + 1];
    cl_event cevent = NULL;
    cl_event revent = NULL;

    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_ONLY, &mat_mem, matp->rows
                             * matp->cols * sizeof(double), NULL);
    if (ret) goto exit;
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_ONLY, &mins_mem, matp->rows
                             * sizeof(double), NULL);
    if (ret) goto free_mat_mem;
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &psums_mem, matp->rows
                             * sizeof(double), NULL);
    if (ret) goto free_mins_mem;
    ret = oclw_set_kernel_arg(reduce_kern, 0, sizeof(matp->cols), &matp->cols,
                              "size");
    if (ret) goto free_psums_mem;
    ret = oclw_set_kernel_arg(reduce_kern, 1, sizeof(cl_mem), &mat_mem, "matrix");
    if (ret) goto free_psums_mem;
    ret = oclw_set_kernel_arg(reduce_kern, 2, sizeof(cl_mem), &psums_mem, "psums");
    if (ret) goto free_psums_mem;
    ret = oclw_set_kernel_arg(reduce_kern, 3, sizeof(cl_mem), &mins_mem, "mins");
    if (ret) goto free_psums_mem;

    // calculate minimums in matrix rows
    for (size_t i = 0; i < matp->rows; ++i) {
        mins[i] = 0.0;
        for (size_t j = 0; j < matp->cols; ++j) {
            ret = double_matrix_get(matp, i, j, &mins[i]);
            if (ret) goto free_psums_mem;
            if (mins[i] > 0.0) break;
        }
        if (mins[i] == 0.0)
            goto free_psums_mem;
        // printf("(%zu, %lf) ", i, mins[i]);
    }
    // printf("\n");

    // fill memory object with matrix
    ret = oclw_async_write_matrix(matp, ocl_queue, mat_mem, matp->rows, wevents);
    if (ret) goto free_psums_mem;
    ret = oclw_async_write_memobj(ocl_queue, mins_mem, 0, sizeof(double) *
                                  matp->rows, mins, wevents + matp->rows);
    if (ret) goto free_psums_mem;

    // run task on memory object after writes
    ret = oclw_async_run_task_after(ocl_queue, reduce_kern, matp->rows, 1,
                                    matp->rows + 1, wevents, &cevent);
    if (ret) goto free_psums_mem;

    // read results after completion
    ret = oclw_async_read_memobj_after(ocl_queue, psums_mem, 0, sizeof(double) *
                                       matp->rows, psums, 1, &cevent, &revent);
    if (ret) goto free_psums_mem;

    // wait till everything is completed
    ret = oclw_wait_till_completion(1, &revent);
    if (ret) goto free_psums_mem;

    // sum all partial sums
    for (size_t i = 0; i < matp->rows; ++i) {
        // printf("%lf ", psums[i]);
        *reduction += psums[i];
    }
    // printf("\n");
free_psums_mem:
    ret |= oclw_destroy_memobj(psums_mem);
free_mins_mem:
    ret |= oclw_destroy_memobj(mins_mem);
free_mat_mem:
    ret |= oclw_destroy_memobj(mat_mem);
exit:
    return ret;
}
#elif defined(USE_PTHREAD)
#include "ptpool.h"
extern struct ptpool* pool;

struct _reduce_vec_args
{
    struct matrix* matp;
    size_t row;
    double X;
};

static void* _reduce_vec_routine(void* args)
{
    struct _reduce_vec_args* targs = (struct _reduce_vec_args*)args;
    double min = 0.0;
    for (size_t j = 0; j < targs->matp->cols; ++j) {
        if (double_matrix_get(targs->matp, targs->row, j, &min))
            return (void*) ((intptr_t) -1);
        if (min > 0.0) break;
    }

    if (min == 0.0)
        return (void*) ((intptr_t) -1);

    for (size_t j = 0; j < targs->matp->cols; ++j) {
        double value = 0.0;
        if (double_matrix_get(targs->matp, targs->row, j, &value))
            return (void*) ((intptr_t) -1);
        targs->X += is_even_as_integer(value / min) ? sin(value) : 0.0;
    }
    return (void*) ((intptr_t) 0);
}

int reduce(struct matrix* matp, double* reduction)
{
    double X = 0.0;
    if (!matp || !reduction) return -1;

    struct _reduce_vec_args tasks_args[matp->rows];
    for (size_t i = 0; i < matp->rows; ++i) {
        tasks_args[i].matp = matp;
        tasks_args[i].row = i;
        tasks_args[i].X = 0.0;
        if (!ptpool_enqueue_task(pool, _reduce_vec_routine, &tasks_args[i]))
            return -1;
    }

    ptpool_wait(pool);
    for (size_t i = 0; i < matp->rows; ++i)
        X += tasks_args[i].X;
    *reduction += X;
    return 0;
}
#elif defined(_OPENMP)
int reduce(struct matrix* matp, double* reduction)
{
    int ret = 0;
    double X = 0.0;
    if (!matp || !reduction) return -1;

    #pragma omp parallel for default(none) shared(matp, ret) reduction(+:X) schedule(runtime)
    for (size_t i = 0; i < matp->rows; ++i) {
        double min = 0.0;
        if (ret) continue;
        for (size_t j = 0; j < matp->cols; ++j) {
            if (double_matrix_get(matp, i, j, &min)) {
                #pragma omp critical
                {
                    ret = -1;
                }
                break;
            }
            if (min > 0.0) break;
        }

        if (!ret && min != 0.0) {
            for (size_t j = 0; j < matp->cols; ++j) {
                double value = 0.0;
                if (double_matrix_get(matp, i, j, &value)) {
                    #pragma omp critical
                    {
                        ret = -1;
                    }
                    break;
                }
                X += is_even_as_integer(value / min) ? sin(value) : 0.0;
            }
            continue;
        }
        #pragma omp critical
        {
            ret = -1;
        }
    }

    if (!ret)
        *reduction = X;
    return ret;
}
#else
int reduce(struct matrix* matp, double* reduction)
{
    int ret = 0;
    if (!matp || !reduction) return -1;
    for (size_t i = 0; i < matp->rows; ++i) {
        double min = 0.0;
        for (size_t j = 0; j < matp->cols; ++j) {
            ret = double_matrix_get(matp, i, j, &min);
            if (ret) return ret;
            if (min > 0.0) break;
        }
        if (min == 0.0) return -1;

        for (size_t j = 0; j < matp->cols; ++j) {
            double value = 0.0;
            ret = double_matrix_get(matp, i, j, &value);
            if (ret) return ret;
            *reduction += is_even_as_integer(value / min) ? sin(value) : 0.0;
        }
    }
    return ret;
}
#endif
