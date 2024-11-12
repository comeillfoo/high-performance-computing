#ifndef _MAPPERS_H_
#define _MAPPERS_H_

#include <stdlib.h>

typedef double (*converter)(double);
double map_sqrt_exp(double number);
double map_abs_ctg(double number);
double map_coth_sqrt(double number);

typedef double (*applicator)(double, double);
double map_abs_sin_sum(double a, double b);

void just_map_matrix(size_t rows, size_t cols, double** matrix, converter fn);

void just_map_matrices(size_t rows, size_t cols, double** src, double** dst,
                       applicator fn);

#ifdef _PTHREAD_H
int parallel_map_array(size_t size, double array[size], mapper fn,
                       size_t threads);
#endif

#endif // _MAPPERS_H_
