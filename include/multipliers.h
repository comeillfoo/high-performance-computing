#ifndef _MULTIPLIERS_H_
#define _MULTIPLIERS_H_

#include <stdlib.h>
#include "matrix.h"

int _multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                       struct matrix* restrict C);

#define multiply_matrices(A, B, C) _multiply_matrices((A), (B), (C))
#endif // _MULTIPLIERS_H_
