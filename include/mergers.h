#ifndef _MERGERS_H_
#define _MERGERS_H_

#include <stdlib.h>
#include "matrix.h"

typedef double (* merger)(double, double);

double merger_pow(double a, double b);

int just_merge_matrices(struct matrix* restrict srcp,
                        struct matrix* restrict dstp, merger fn);

#endif // _MERGERS_H_
