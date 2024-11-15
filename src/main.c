#include <stdio.h>
#include <errno.h>

#include "matrix.h"
#include "generators.h"
#include "mappers.h"
#include "mergers.h"
#include "multipliers.h"
#include "sorts.h"
#include "reducers.h"


static int args_parse(int argc, char* argv[], int* Np);

int main(int argc, char* argv[])
{
    const double A = 400.0; // А = Ф(8) * И(5) * О(10)
    int N, ret = 0;

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
        ret = generate_random_matrix(&M1, 1.0, A, i);
        if (ret) goto freeM;
        ret = generate_random_matrix(&M2, A, 10.0 * A, i);
        if (ret) goto freeM;

        // Map. В матрице M1 к каждому элементу применить операцию из таблицы.
        // В матрице M2 каждую колонку поочередно сложить с предыдущей
        ret = map_matrix(&M1, apply_coth_sqrt);
        if (ret) goto freeM;
        ret = shift_matrices(&M2, &Mt, 1);
        if (ret) goto freeM;
        ret = map_matrices(&Mt, &M2, combine_abs_sin_sum);
        if (ret) goto freeM;

        // Merge. В матрицах М1 и М2 ко всем элементами с одинаковыми индексами
        // попарно применить операцию из таблицы (результат записать в М2).
        ret = merge_matrices(&M1, &M2, merge_by_pow);
        if (ret) goto freeM;

        // Multiply. Умножить матрицы M1 и M2
        // M1[N][N / 2] * M2[N / 2][N] = M[N][N]
        ret = multiply_matrices(&M1, &M2, &M);
        if (ret) goto freeM;

        // Sort. Полученную матрицу M необходимо отсортировать по строкам
        ret = sort_rows(&M);
        if (ret) goto freeM;

        // Reduce. Рассчитать сумму синусов тех элементов матрицы M, которые при
        // делении на минимальный ненулевой элемент массива соответствующей строки дают четное число
        double X = 0.0;
        ret = reduce(&M, &X);
        if (ret) goto freeM;
        printf("X = %lf\n", X);
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
