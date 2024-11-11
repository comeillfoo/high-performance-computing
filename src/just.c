#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include "just.h"


static inline double random_double_r(double a, double b, unsigned* seedp)
{
    return a + ((double)rand_r(seedp)) / (((double) RAND_MAX) / (b - a));
}

bool is_even(double number)
{
    return !(((uint_least64_t) number) % 2);
}

int generate_random_uniform_array(size_t size, double array[size],
                                  double a, double b, unsigned seed)
{
    if (!array) return EINVAL;

    for (size_t i = 0; i < size; ++i)
        array[i] = random_double_r(a, b, &seed);
    return 0;
}

int alloc_double_matrix(double*** Mppp, size_t rows, size_t cols)
{
    if (!Mppp) return -1;
    *Mppp = malloc(rows * sizeof(double*));
    if (!*Mppp) return -1;
    for (size_t i = 0; i < rows; ++i) {
        (*Mppp)[i] = malloc(cols * sizeof(double));
        if ((*Mppp)[i]) continue;
        for (size_t j = 0; j < i; ++j)
            free((*Mppp)[i]);
        free(*Mppp);
        return -1;
    }
    return 0;
}
