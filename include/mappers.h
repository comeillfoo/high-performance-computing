#ifndef _MAPPERS_H_
#define _MAPPERS_H_

#include <stdlib.h>
#include "matrix.h"

typedef double (*applicator)(double);
double apply_sqrt_exp(double number);
double apply_abs_ctg(double number);
double apply_coth_sqrt(double number);

typedef double (*combiner)(double, double);
double combine_abs_sin_sum(double a, double b);

int _map_matrix(struct matrix* matp, applicator fn);
int _map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                 combiner fn);
int _shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   size_t shift);

#define map_matrix(matp, fn) _map_matrix((matp), (fn))
#define map_matrices(srcp, dstp, fn) _map_matrices((srcp), (dstp), (fn))
#define shift_matrices(srcp, dstp, shift) _shift_matrices((srcp), (dstp), (shift))
#endif // _MAPPERS_H_
