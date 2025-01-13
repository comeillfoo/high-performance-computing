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

extern cl_command_queue ocl_queue;
cl_kernel reduce_kern = NULL;

int _reduce(struct matrix* matp, double* reduction, cl_mem* mat_memp,
            cl_mem* psums_memp, cl_event event)
{
    int ret = 0;
    if (!matp || !reduction) return -1;
    if (!matp->rows || !matp->cols) return 0;
    if (!ocl_queue || !mat_memp || !psums_memp || !event || !reduce_kern)
        return -1;

    double psums[matp->rows];
    cl_event cevent = NULL;
    cl_event revent = NULL;
    ret = oclw_set_kernel_arg(reduce_kern, 0, sizeof(matp->cols), &matp->cols,
                              "size");
    if (ret) goto exit;
    ret = oclw_set_kernel_arg(reduce_kern, 1, sizeof(cl_mem), mat_memp, "matrix");
    if (ret) goto exit;
    ret = oclw_set_kernel_arg(reduce_kern, 2, sizeof(cl_mem), psums_memp, "psums");
    if (ret) goto exit;

    // run task on memory object after writes
    ret = oclw_async_run_task_after(ocl_queue, reduce_kern, matp->rows, NULL, 1,
                                    &event, &cevent);
    if (ret) goto exit;

    // read results after completion
    ret = oclw_async_read_memobj_after(ocl_queue, *psums_memp, 0, sizeof(double) *
                                       matp->rows, psums, 1, &cevent, &revent);
    if (ret) goto exit;

    // wait till everything is completed
    ret = oclw_wait_till_completion(1, &revent);
    if (ret) goto exit;

    // sum all partial sums
    for (size_t i = 0; i < matp->rows; ++i) {
        *reduction += psums[i];
    }
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

int _reduce(struct matrix* matp, double* reduction)
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
int _reduce(struct matrix* matp, double* reduction)
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
int _reduce(struct matrix* matp, double* reduction)
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
