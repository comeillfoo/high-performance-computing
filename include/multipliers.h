#ifndef _MULTIPLIERS_H_
#define _MULTIPLIERS_H_

#include <stdlib.h>
#include "matrix.h"

#ifdef USE_OPENCL
int _multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                       struct matrix* restrict C, cl_mem* restrict A_memp,
                       cl_mem* restrict B_memp, cl_mem* restrict C_memp,
                       cl_event event, cl_event* eventp);
#else
int _multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                       struct matrix* restrict C);
#endif

#ifdef USE_OPENCL
#define multiply_matrices(A, B, C) _multiply_matrices((A), (B), (C), (A##_mem), \
                                                      (B##_mem), (C##_mem), \
                                                      merge_matrices_event, \
                                                      &multiply_matrices_event)
#else
#define multiply_matrices(A, B, C) _multiply_matrices((A), (B), (C))
#endif
#endif // _MULTIPLIERS_H_
