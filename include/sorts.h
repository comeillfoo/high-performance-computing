#ifndef _SORTS_H_
#define _SORTS_H_

#include "matrix.h"

int _sort_rows(struct matrix* matp);

#define sort_rows(matp) _sort_rows((matp))
#endif // _SORTS_H_
