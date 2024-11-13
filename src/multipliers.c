#include "multipliers.h"


int multiply_matrices(struct matrix* restrict A, struct matrix* restrict B,
                           struct matrix* restrict C)
{
    int ret = 0;
    if (!A || !B || !C || A->cols != B->rows || A->rows != C->rows
        || B->cols != C->cols)
        return -1;
    // A[a][b] * B[b][c] = C[a][c]
    for (size_t i = 0; i < A->rows; ++i)
        for (size_t j = 0; j < B->cols; ++j) {
            ret = double_matrix_set(C, i, j, 0.0);
            if (ret) return ret;
            for (size_t k = 0; k < B->rows; ++k) {
                double a = 0.0;
                double b = 0.0;
                double c = 0.0;
                ret = double_matrix_get(A, i, k, &a);
                if (ret) return ret;
                ret = double_matrix_get(B, k, j, &b);
                if (ret) return ret;
                ret = double_matrix_get(C, i, j, &c);
                if (ret) return ret;
                ret = double_matrix_set(C, i, j, c + a * b);
                if (ret) return ret;
            }
        }
    return ret;
}
