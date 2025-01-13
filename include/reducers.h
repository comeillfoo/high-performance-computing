#ifndef _REDUCERS_H_
#define _REDUCERS_H_

#include "matrix.h"

#ifdef USE_OPENCL
int _reduce(struct matrix* matp, double* reduction, cl_mem* mat_memp,
            cl_mem* psums_memp, cl_event event);
#else
int _reduce(struct matrix* matp, double* reduction);
#endif

#ifdef USE_OPENCL
#define reduce(matp, reduction) _reduce((matp), (reduction), (matp##_mem), \
                                        (matp##_psums_mem), sort_rows_event)
#else
#define reduce(matp, reduction) _reduce((matp), (reduction))
#endif
#endif // _REDUCERS_H_
