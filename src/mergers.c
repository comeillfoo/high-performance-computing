#include "mergers.h"

#include <math.h>

double merge_by_pow(double a, double b)
{
    return pow(a, b);
}

#define MIN(a, b) (((a) < (b))? (a) : (b))

#ifdef USE_OPENCL
#include "oclw.h"

extern cl_context ocl_context;
extern cl_command_queue ocl_queue;

cl_kernel merge_by_pow_kern = NULL;


int merge_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   merger fn)
{
    int ret = 0;
    cl_mem src_mem = NULL, dst_mem = NULL;
    cl_kernel ocl_kern = NULL;
    if (!srcp || !dstp) return -1;
    const size_t rows = MIN(srcp->rows, dstp->rows);
    const size_t cols = MIN(srcp->cols, dstp->cols);
    if (!rows || !cols) return 0;

    if (fn == merge_by_pow)
        ocl_kern = merge_by_pow_kern;
    if (!ocl_context || !ocl_queue || !ocl_kern) return -1;

    cl_event wevents[rows << 1];
    cl_event cevent = NULL;
    cl_event revents[rows];

    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_ONLY, &src_mem, rows *
                             srcp->cols * sizeof(double), NULL);
    if (ret) goto exit;
    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &dst_mem, rows *
                             dstp->cols * sizeof(double), NULL);
    if (ret) goto free_src_mem;
    ret = oclw_set_kernel_arg(ocl_kern, 0, sizeof(srcp->cols), &srcp->cols,
                              "src_cols");
    if (ret) goto free_dst_mem;
    ret = oclw_set_kernel_arg(ocl_kern, 1, sizeof(cl_mem), &src_mem, "source");
    if (ret) goto free_dst_mem;
    ret = oclw_set_kernel_arg(ocl_kern, 2, sizeof(dstp->cols), &dstp->cols,
                              "dst_cols");
    if (ret) goto free_dst_mem;
    ret = oclw_set_kernel_arg(ocl_kern, 3, sizeof(cl_mem), &dst_mem, "target");
    if (ret) goto free_dst_mem;

    // fill memory objects with matrices
    ret = oclw_async_write_matrix(srcp, ocl_queue, src_mem, rows, wevents);
    if (ret) goto free_dst_mem;
    ret = oclw_async_write_matrix(dstp, ocl_queue, dst_mem, rows, wevents + rows);
    if (ret) goto free_dst_mem;

    // run task on memory objects after writes
    const size_t global_work_size[2] = { rows, cols };
    const size_t local_work_size[2] = { 1, 1 };
    cl_int cl_ret = clEnqueueNDRangeKernel(ocl_queue, merge_by_pow_kern, 2,
                                           NULL, global_work_size,
                                           local_work_size, rows << 1, wevents,
                                           &cevent);
    if (cl_ret != CL_SUCCESS) {
        oclw_error(cl_ret, "Unable to merge matrices");
        ret = -1;
    }
    if (ret) goto free_dst_mem;
    if (ret) goto free_dst_mem;

    // read results into original matrix after completion
    ret = oclw_async_read_matrix(dstp, ocl_queue, dst_mem, rows, revents, &cevent);
    if (ret) goto free_dst_mem;

    // wait till everything is completed
    ret = oclw_wait_till_completion(rows, revents);
free_dst_mem:
    ret |= oclw_destroy_memobj(dst_mem);
free_src_mem:
    ret |= oclw_destroy_memobj(src_mem);
exit:
    return ret;
}
#elif defined(USE_PTHREAD)
#include <stdint.h>
#include "ptpool.h"
extern struct ptpool* pool;

struct _merge_matrices_args
{
    struct matrix* srcp;
    struct matrix* dstp;
    size_t row;
    size_t col;
    merger fn;
};

static void* _merge_matrices_routine(void* args)
{
    struct _merge_matrices_args* targs = (struct _merge_matrices_args*)args;
    double a = 0.0;
    double b = 0.0;
    if (double_matrix_get(targs->srcp, targs->row, targs->col, &a))
        return (void*) ((intptr_t) -1);
    if (double_matrix_get(targs->dstp, targs->row, targs->col, &b))
        return (void*) ((intptr_t) -1);
    if (double_matrix_set(targs->dstp, targs->row, targs->col, targs->fn(a, b)))
        return (void*) ((intptr_t) -1);

    return (void*) ((intptr_t) 0);
}

int merge_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   merger fn)
{
    if (!srcp || !dstp) return -1;
    const size_t rows = MIN(srcp->rows, dstp->rows);
    const size_t cols = MIN(srcp->cols, dstp->cols);
    size_t k = 0;
    struct _merge_matrices_args tasks_args[rows * cols];
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j) {
            tasks_args[k].srcp = srcp;
            tasks_args[k].dstp = dstp;
            tasks_args[k].row = i;
            tasks_args[k].col = j;
            tasks_args[k].fn = fn;
            if (!ptpool_enqueue_task(pool, _merge_matrices_routine, &tasks_args[k]))
                return -1;
            k++;
        }

    ptpool_wait(pool);
    return 0;
}
#elif defined(_OPENMP)
int merge_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   merger fn)
{
    int ret = 0;
    if (!srcp || !dstp) return -1;
    const size_t rows = MIN(srcp->rows, dstp->rows);
    const size_t cols = MIN(srcp->cols, dstp->cols);
    #pragma omp parallel for collapse(2) default(none) shared(srcp, dstp, ret, fn, rows, cols) schedule(runtime)
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j) {
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
            if (double_matrix_set(dstp, i, j, fn(a, b))) {
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
int merge_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   merger fn)
{
    int ret = 0;
    if (!srcp || !dstp) return -1;
    for (size_t i = 0; i < MIN(srcp->rows, dstp->rows); ++i)
        for (size_t j = 0; j < MIN(srcp->cols, dstp->cols); ++j) {
            double a = 0.0;
            double b = 0.0;
            ret = double_matrix_get(srcp, i, j, &a);
            if (ret) return ret;
            ret = double_matrix_get(dstp, i, j, &b);
            if (ret) return ret;
            ret = double_matrix_set(dstp, i, j, fn(a, b));
            if (ret) return ret;
        }
    return ret;
}
#endif
#undef MIN
