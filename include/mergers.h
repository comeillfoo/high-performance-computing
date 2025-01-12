#ifndef _MERGERS_H_
#define _MERGERS_H_

#include <stdlib.h>
#include "matrix.h"

typedef double (* merger)(double, double);

double merge_by_pow(double a, double b);

int _merge_matrices(struct matrix* restrict srcp,
                    struct matrix* restrict dstp, merger fn);

#define merge_matrices(srcp, dstp, fn) _merge_matrices((srcp), (dstp), (fn))
#endif // _MERGERS_H_
