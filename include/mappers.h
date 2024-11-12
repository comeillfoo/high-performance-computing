#ifndef _MAPPERS_H_
#define _MAPPERS_H_

#include <stdlib.h>
#include "matrix.h"

typedef double (*converter)(double);
double map_sqrt_exp(double number);
double map_abs_ctg(double number);
double map_coth_sqrt(double number);

typedef double (*applicator)(double, double);
double map_abs_sin_sum(double a, double b);

int just_map_matrix(struct matrix* matp, converter fn);
int just_map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                      applicator fn);

#ifdef _PTHREAD_H
int parallel_map_array(size_t size, double array[size], mapper fn,
                       size_t threads);
#endif

#endif // _MAPPERS_H_
