#include "matrix.h"


int double_matrix_create(size_t rows, size_t cols, enum matrix_type type,
                         struct matrix* matp)
{
    if (!matp) return -1;
    matp->rows = rows;
    matp->cols = cols;
    matp->type = type;

    switch (matp->type) {
    case MT_VECTOR:
        matp->as_vector = malloc(rows * cols * sizeof(double));
        if (!matp->as_vector) return -1;
        break;
    case MT_TABLE:
        matp->as_table = malloc(rows * sizeof(double*));
        if (!matp->as_table) return -1;
        for (size_t i = 0; i < rows; ++i) {
            matp->as_table[i] = malloc(cols * sizeof(double));
            if (matp->as_table[i]) continue;
            for (size_t j = 0; j < i; ++j)
                free(matp->as_table[j]);
            free(matp->as_table);
            return -1;
        }
        break;
    default:
        break;
    }
    return 0;
}

void double_matrix_destroy(struct matrix mat)
{
    switch (mat.type) {
    case MT_VECTOR:
        free(mat.as_vector);
        break;
    case MT_TABLE:
        for (size_t i = 0; i < mat.rows; ++i)
            free(mat.as_table[i]);
        free(mat.as_table);
        break;
    default:
        break;
    }
}

int double_matrix_get(struct matrix* matp, size_t row, size_t col,
                      double* valup)
{
    if (!valup || !matp) return -1;
    if (matp->rows <= row || matp->cols <= col)
        return -1;

    switch (matp->type) {
    case MT_VECTOR:
        *valup = matp->as_vector[matp->cols * row + col];
        break;
    case MT_TABLE:
        *valup = matp->as_table[row][col];
        break;
    default:
        break;
    }
    return 0;
}

int double_matrix_set(struct matrix* matp, size_t row, size_t col,
                      double value)
{
    if (matp->rows <= row || matp->cols <= col)
        return -1;
    switch (matp->type) {
    case MT_VECTOR:
        matp->as_vector[matp->cols * row + col] = value;
        break;
    case MT_TABLE:
        matp->as_table[row][col] = value;
        break;
    default:
        break;
    }
    return 0;
}
