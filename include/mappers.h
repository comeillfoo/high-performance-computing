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

int map_matrix(struct matrix* matp, applicator fn);
int map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                 combiner fn);
int shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   size_t shift);

#ifdef _PTHREAD_H
int parallel_map_array(size_t size, double array[size], mapper fn,
                       size_t threads);
#endif

#endif // _MAPPERS_H_
