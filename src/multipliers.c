#include "multipliers.h"

void just_multiply_matrices(size_t m, size_t n, double** restrict A,
                            double** restrict B, double** restrict C)
{
    // A[m][n] * B[n][m] = C[m][m]
    for (size_t i = 0; i < m; ++i)
        for (size_t j = 0; j < m; ++j) {
            C[i][j] = 0;
            for (size_t k = 0; k < n; ++k)
                C[i][j] += A[i][k] * B[k][j];
        }
}
