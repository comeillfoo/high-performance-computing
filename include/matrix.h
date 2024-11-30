#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <stdlib.h>

enum matrix_type {
    MT_VECTOR = 0,
    MT_TABLE
};

#ifdef USE_MT_TABLE
#define MT_DEFAULT_TYPE (MT_TABLE)
#elif defined(USE_MT_VECTOR)
#define MT_DEFAULT_TYPE (MT_VECTOR)
#else
#define MT_DEFAULT_TYPE (MT_TABLE)
#endif

struct matrix {
    size_t rows;
    size_t cols;
    enum matrix_type type;
    union {
        double* as_vector;
        double** as_table;
    };
};

int double_matrix_create(size_t rows, size_t cols, enum matrix_type type,
                         struct matrix* matp);
void double_matrix_destroy(struct matrix matrix);
int double_matrix_get(struct matrix* matp, size_t row, size_t col,
                      double* valup);
int double_matrix_set(struct matrix* matp, size_t row, size_t col,
                      double value);

#endif // _MATRIX_H_
