#include "generators.h"

#include <errno.h>
#include <time.h>


static inline double random_double_r(double a, double b, unsigned* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

#ifdef _OPENMP
#include <omp.h>
int generate_random_matrix(struct matrix* matp, double a, double b, unsigned seed)
{
    int ret = 0;
    if (!matp) return -1;
    unsigned* seeds = NULL;

    #pragma omp parallel default(none) shared(matp, seeds, ret, a, b, seed)
    {
        #pragma omp single
        {
            seeds = malloc(sizeof(unsigned) * omp_get_num_threads());
            if (seeds) {
                seeds[0] = seed;
                for (size_t i = 1; i < omp_get_num_threads(); ++i)
                    seeds[i] = time(NULL); // that may be unsafe way
            } else {
                ret = -1;
            }
        }

        if (ret) {
            #pragma omp cancel parallel
        }

        #pragma omp for
        for (size_t i = 0; i < matp->rows; ++i)
            for (size_t j = 0; j < matp->cols; ++j) {
                int lret = double_matrix_set(matp, i, j,
                                             random_double_r(a, b, &seed));
                if (lret) {
                    #pragma omp critical
                    {
                        ret = lret;
                    }
                    #pragma omp cancel for
                }
            }

        #pragma omp single
        {
            free(seeds);
        }
    }
    return ret;
}
#else
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
#endif
