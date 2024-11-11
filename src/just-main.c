#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "just.h"


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

    ret = args_parse(argc, argv, &N);
    if (ret) goto exit;

    double** M1 = NULL;
    ret = alloc_double_matrix(&M1, N, N / 2);
    if (ret) goto exit;

    double** M2 = NULL;
    ret = alloc_double_matrix(&M2, N / 2, N);
    if (ret) goto freeM1;

    for (size_t i = 0; i < (N / 2); ++i)
        free(M2[i]);
    free(M2);
freeM1:
    for (size_t i = 0; i < N; ++i)
        free(M1[i]);
    free(M1);
exit:
    return ret;
}
