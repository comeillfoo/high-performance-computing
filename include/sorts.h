#ifndef _SORTS_H_
#define _SORTS_H_

#include "matrix.h"

#ifdef USE_OPENCL
int _sort_rows(struct matrix* matp, cl_mem* ocl_memp, cl_event event,
               cl_event* eventp);
#else
int _sort_rows(struct matrix* matp);
#endif

#ifdef USE_OPENCL
#define sort_rows(matp) _sort_rows((matp), (matp##_mem), multiply_matrices_event, \
                                   &sort_rows_event)
#else
#define sort_rows(matp) _sort_rows((matp))
#endif
#endif // _SORTS_H_
