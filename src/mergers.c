#include "mergers.h"

#include <math.h>

double merge_by_pow(double a, double b)
{
    return pow(a, b);
}

#define MIN(a, b) (((a) < (b))? (a) : (b))

#ifdef USE_PTHREAD
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
