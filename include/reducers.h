#ifndef _REDUCERS_H_
#define _REDUCERS_H_

#include "matrix.h"

int _reduce(struct matrix* matp, double* reduction);

#define reduce(matp, reduction) _reduce((matp), (reduction))
#endif // _REDUCERS_H_
