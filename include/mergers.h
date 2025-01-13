#ifndef _MERGERS_H_
#define _MERGERS_H_

#include <stdlib.h>
#include "matrix.h"

typedef double (* merger)(double, double);

double merge_by_pow(double a, double b);

#ifdef USE_OPENCL
int _merge_matrices(struct matrix* restrict srcp, struct matrix* restrict dstp,
                    cl_mem* restrict src_memp, cl_mem* restrict dst_memp,
                    cl_kernel ocl_kern, size_t num_events, cl_event* events,
                    cl_event* eventp);
#else
int _merge_matrices(struct matrix* restrict srcp,
                    struct matrix* restrict dstp, merger fn);
#endif

#ifdef USE_OPENCL
#define merge_matrices(srcp, dstp, fn) _merge_matrices((srcp), (dstp), \
                                                       (srcp##_mem), \
                                                       (dstp##_mem), (fn##_kern), \
                                                       2, (cl_event[2]){ map_matrix_event, \
                                                       map_matrices_event }, \
                                                       &merge_matrices_event)
#else
#define merge_matrices(srcp, dstp, fn) _merge_matrices((srcp), (dstp), (fn))
#endif
#endif // _MERGERS_H_
