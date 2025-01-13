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

#ifdef USE_OPENCL
int _map_matrix(struct matrix* matp, cl_mem* ocl_memp, cl_kernel ocl_kern,
                cl_event* eventp);
int _shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                    cl_mem* restrict src_memp, cl_mem* restrict dst_memp,
                    size_t shift, cl_event* eventp);
int _map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                  cl_mem* restrict src_memp, cl_mem* restrict dst_memp,
                  cl_kernel ocl_kern, cl_event event, cl_event* eventp);
#else
int _map_matrix(struct matrix* matp, applicator fn);
int _shift_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                   size_t shift);
int _map_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                 combiner fn);
#endif


#ifdef USE_OPENCL
#define map_matrix(matp, fn) _map_matrix((matp), (matp##_mem), (fn##_kern), \
                                         &map_matrix_event)
#define shift_matrices(srcp, dstp, shift) _shift_matrices((srcp), (dstp), \
                                                          (srcp##_mem), \
                                                          (dstp##_mem), (shift), \
                                                          &shift_matrices_event)
#define map_matrices(srcp, dstp, fn) _map_matrices((srcp), (dstp), (srcp##_mem), \
                                                   (dstp##_mem), fn##_kern, \
                                                   (shift_matrices_event), \
                                                   &map_matrices_event)
#else
#define map_matrix(matp, fn) _map_matrix((matp), (fn))
#define shift_matrices(srcp, dstp, shift) _shift_matrices((srcp), (dstp), (shift))
#define map_matrices(srcp, dstp, fn) _map_matrices((srcp), (dstp), (fn))
#endif

#endif // _MAPPERS_H_
