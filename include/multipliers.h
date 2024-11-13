#ifndef _MULTIPLIERS_H_
#define _MULTIPLIERS_H_

#include <stdlib.h>
#include "matrix.h"

int multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                           struct matrix* restrict C);

#endif // _MULTIPLIERS_H_
