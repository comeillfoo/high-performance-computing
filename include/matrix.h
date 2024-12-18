#ifndef _MATRIX_H_
#define _MATRIX_H_

#include <stdlib.h>

enum matrix_type {
    MT_VECTOR = 0,
    MT_TABLE
};

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
double* double_matrix_get_row_mut(struct matrix* matp, size_t row);

#endif // _MATRIX_H_
