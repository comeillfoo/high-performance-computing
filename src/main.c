#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "generators.h"
#include "mappers.h"
#include "mergers.h"
#include "multipliers.h"


static int alloc_double_matrix(double*** Mp, size_t rows, size_t cols);
static void free_double_matrix(double** M, size_t rows);
static bool is_even(double number);
static int args_parse(int argc, char* argv[], int* Np)
{
    int N = 0, ret = 0;
    if (argc < 2 || !Np) {
        ret = EINVAL;
        goto usage;
    }
    N = strtol(argv[1], NULL, 10);
    ret = errno || (N <= 1);
    if (ret)
        goto usage;

    *Np = N;
    return 0;
usage:
    printf("Usage: %s N\n\nArguments:\n    N    size of matrices, default 2\n",
           argv[0]);
    return ret;
}

int main(int argc, char* argv[])
{
    const double A = 400.0; // А = Ф(8) * И(5) * О(10)
    int N, ret = 0;
    (void)is_even;

    ret = args_parse(argc, argv, &N);
    if (ret) goto exit;

    double** M1 = NULL;
    ret = alloc_double_matrix(&M1, N, N / 2);
    if (ret) goto exit;

    double** M2 = NULL;
    ret = alloc_double_matrix(&M2, N / 2, N);
    if (ret) goto freeM1;

    double** Mt = NULL;
    ret = alloc_double_matrix(&Mt, N / 2, N);
    if (ret) goto freeM2;

    double** M = NULL;
    ret = alloc_double_matrix(&M, N, N);
    if (ret) goto freeMt;

    for (size_t i = 0; i < 100; ++i) {
        // Generate. Сформировать матрицу M1[N][N / 2], заполнить uniform(1, A).
        // Сформировать матрицу M2[N / 2][N], заполнить uniform(A, 10.0 * A).
        ret = just_generate_random_matrix(N, N / 2, M1, 1.0, A, i);
        if (ret) goto freeM;
        ret = just_generate_random_matrix(N / 2, N, M2, A, 10.0 * A, i);
        if (ret) goto freeM;

        // Map. В матрице M1 к каждому элементу применить операцию из таблицы.
        // В матрице M2 каждую колонку поочередно сложить с предыдущей
        just_map_matrix(N, N / 2, M1, map_coth_sqrt);
        for (size_t i = 0; i < (N / 2); ++i)
            for (size_t j = 0; j < N; ++j)
                Mt[i][(j + 1) % N] = M2[i][j];
        just_map_matrices(N / 2, N, Mt, M2, map_abs_sin_sum);

        // Merge. В матрицах М1 и М2 ко всем элементами с одинаковыми индексами
        // попарно применить операцию из таблицы (результат записать в М2).
        just_merge_matrices(N / 2, N / 2, M1, M2, merger_pow_rev);

        // Multiply. Умножить матрицы M1 и M2
        // M1[N][N / 2] * M2[N / 2][N] = M[N][N]
        just_multiply_matrices(N, N / 2, M1, M2, M);
    }

freeM:
    free_double_matrix(M, N);
freeMt:
    free_double_matrix(Mt, N / 2);
freeM2:
    free_double_matrix(M2, N / 2);
freeM1:
    free_double_matrix(M1, N);
exit:
    return ret;
}

static int alloc_double_matrix(double*** Mp, size_t rows, size_t cols)
{
    if (!Mp) return -1;
    *Mp = malloc(rows * sizeof(double*));
    if (!*Mp) return -1;
    for (size_t i = 0; i < rows; ++i) {
        (*Mp)[i] = malloc(cols * sizeof(double));
        if ((*Mp)[i]) continue;
        for (size_t j = 0; j < i; ++j)
            free((*Mp)[i]);
        free(*Mp);
        return -1;
    }
    return 0;
}

static void free_double_matrix(double** M, size_t rows)
{
    if (!M) return;
    for (size_t i = 0; i < rows; ++i)
        free(M[i]);
    free(M);
}

static bool is_even(double number)
{
    return !(((uint_least64_t) number) % 2);
}
