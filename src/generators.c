#include "generators.h"

#include <errno.h>


static inline double random_double_r(double a, double b, unsigned* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

int generate_random_matrix(struct matrix* matp, double a, double b, unsigned seed)
{
    int ret = 0;
    if (!matp) return -1;
    for (size_t i = 0; i < matp->rows; ++i)
        for (size_t j = 0; j < matp->cols; ++j) {
            ret = double_matrix_set(matp, i, j, random_double_r(a, b, &seed));
            if (ret) return ret;
        }
    return ret;
}
