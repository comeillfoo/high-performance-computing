#include "generators.h"

#include <errno.h>


static inline double random_double_r(double a, double b, unsigned* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

int just_generate_random_matrix(size_t rows, size_t cols, double** matrix,
                                double a, double b, unsigned seed)
{
    if (!matrix) return EINVAL;

    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            matrix[i][j] = random_double_r(a, b, &seed);
    return 0;
}
