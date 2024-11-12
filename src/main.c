#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "matrix.h"
#include "generators.h"
#include "mappers.h"
#include "mergers.h"
#include "multipliers.h"


static bool is_even(double number);
static int args_parse(int argc, char* argv[], int* Np);

int main(int argc, char* argv[])
{
    const double A = 400.0; // А = Ф(8) * И(5) * О(10)
    int N, ret = 0;
    (void)is_even;

    ret = args_parse(argc, argv, &N);
    if (ret) goto exit;

    struct matrix M1 = {0};
    ret = double_matrix_create(N, N / 2, MT_TABLE, &M1);
    if (ret) goto exit;

    struct matrix M2 = {0};
    ret = double_matrix_create(N / 2, N, MT_TABLE, &M2);
    if (ret) goto freeM1;

    struct matrix Mt = {0};
    ret = double_matrix_create(N / 2, N, MT_TABLE, &Mt);
    if (ret) goto freeM2;

    struct matrix M = {0};
    ret = double_matrix_create(N, N, MT_TABLE, &M);
    if (ret) goto freeMt;

    for (size_t i = 0; i < 100; ++i) {
        // Generate. Сформировать матрицу M1[N][N / 2], заполнить uniform(1, A).
        // Сформировать матрицу M2[N / 2][N], заполнить uniform(A, 10.0 * A).
        ret = just_generate_random_matrix(&M1, 1.0, A, i);
        if (ret) goto freeM;
        ret = just_generate_random_matrix(&M2, A, 10.0 * A, i);
        if (ret) goto freeM;

        // Map. В матрице M1 к каждому элементу применить операцию из таблицы.
        // В матрице M2 каждую колонку поочередно сложить с предыдущей
        ret = just_map_matrix(&M1, map_coth_sqrt);
        if (ret) goto freeM;
        for (size_t i = 0; i < M2.rows; ++i)
            for (size_t j = 0; j < M2.cols; ++j) {
                double m2 = 0.0;
                ret = double_matrix_get(&M2, i, j, &m2);
                if (ret) goto freeM;
                ret = double_matrix_set(&Mt, i, (j + 1) % M2.cols, m2);
                if (ret) goto freeM;
            }
        ret = just_map_matrices(&Mt, &M2, map_abs_sin_sum);
        if (ret) goto freeM;

        // Merge. В матрицах М1 и М2 ко всем элементами с одинаковыми индексами
        // попарно применить операцию из таблицы (результат записать в М2).
        ret = just_merge_matrices(&M1, &M2, merger_pow);
        if (ret) goto freeM;

        // Multiply. Умножить матрицы M1 и M2
        // M1[N][N / 2] * M2[N / 2][N] = M[N][N]
        ret = just_multiply_matrices(&M1, &M2, &M);
        if (ret) goto freeM;
    }

freeM:
    double_matrix_destroy(M);
freeMt:
    double_matrix_destroy(Mt);
freeM2:
    double_matrix_destroy(M2);
freeM1:
    double_matrix_destroy(M1);
exit:
    return ret;
}

static bool is_even(double number)
{
    return !(((uint_least64_t) number) % 2);
}

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
