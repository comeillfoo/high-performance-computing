#include "reducers.h"

#include <stdbool.h>
#include <stdint.h>
#include <math.h>


static bool is_even_as_integer(double number)
{
    return !(((uint_least64_t) number) % 2);
}


#ifdef _OPENMP
int reduce(struct matrix* matp, double* reduction)
{
    int ret = 0;
    double X = 0.0;
    if (!matp || !reduction) return -1;

    #pragma omp parallel for default(none) shared(matp, ret) reduction(+:X)
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
