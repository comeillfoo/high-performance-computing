#include "mergers.h"

#include <math.h>

double merge_by_pow(double a, double b)
{
    return pow(a, b);
}

#define MIN(a, b) (((a) < (b))? (a) : (b))
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
#undef MIN
