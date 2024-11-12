#ifndef _MERGERS_H_
#define _MERGERS_H_

#include <stdlib.h>

typedef double (* merger)(double, double);

double merger_pow_rev(double a, double b);

void just_merge_matrices(size_t rows, size_t cols, double** restrict src,
                         double** restrict dst, merger fn);

#endif // _MERGERS_H_
