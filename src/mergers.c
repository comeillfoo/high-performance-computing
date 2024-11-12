#include "mergers.h"

#include <math.h>

double merger_pow_rev(double a, double b)
{
    return pow(b, a);
}

void just_merge_matrices(size_t rows, size_t cols, double** restrict src,
                         double** restrict dst, merger fn)
{
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            dst[i][j] = fn(dst[i][j], src[i][j]);
}
