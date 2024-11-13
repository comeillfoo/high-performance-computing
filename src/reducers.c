#include "reducers.h"

#include <stdbool.h>
#include <stdint.h>
#include <math.h>


static bool is_even_as_integer(double number)
{
    return !(((uint_least64_t) number) % 2);
}

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
            if (is_even_as_integer(value / min))
                *reduction += sin(value);
        }
    }
    return ret;
}
