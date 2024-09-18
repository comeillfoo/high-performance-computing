#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <omp.h>

#include "mappers.h"
#include "sorts.h"

#ifndef __GNUC__

// here we assume that unsigned is 32bit integer
static unsigned rand_r(unsigned *seed)
{
	(*seed) ^= (*seed) >> 11;
	(*seed) ^= (*seed) << 7 & 0x9D2C5680;
	(*seed) ^= (*seed) << 15 & 0xEFC60000;
	(*seed) ^= (*seed) >> 18;
	return (*seed);
}

#endif

static inline bool is_even(double number)
{
    return !(((uint_least64_t) number) % 2);
}

static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, unsigned seed);

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
    const double A = 450.0; // А = Ф * И * О
    int N, ret = 0;
    double prev, curr, X;
    size_t i, M;
    double T1, T2;
    long delta_ms;

    ret = args_parse(argc, argv, &N); // N равен первому параметру командной строки
    if (ret)
        goto exit;
    M = N / 2;

    double* M1 = malloc(N * sizeof(double));
    double* M2 = malloc(M * sizeof(double));

    T1 = omp_get_wtime(); // запомнить текущее время T1
    for (i = 0; i < 100; ++i) { // 100 экспериментов
        // Generate. Заполнить массив исходных данных размером N
        ret = generate_random_uniform_array(N, M1, 1.0, A, i);
        if (ret) goto freeMs;
        ret = generate_random_uniform_array(M, M2, A, 10.0 * A, i);
        if (ret) goto freeMs;

        // Map. Решить поставленную задачу, заполнить массив с результатами
        #pragma omp parallel for default(none) shared(N, M1)
        for (size_t j = 0; j < N; ++j)
            M1[j] = map_sqrt_exp(M1[j]);

        prev = M2[0];
        M2[0] = map_abs_ctg(M2[0]);
        // no pragmas because: of read/write dependencies
        for (size_t j = 1; j < M; ++j) {
            curr = map_abs_ctg(M2[j] + prev);
            prev = M2[j];
            M2[j] = curr;
        }

        #pragma omp parallel for default(none) shared(M1, M2, M)
        for (size_t j = 0; j < M; ++j)
            M2[j] = (M1[j] > M2[j]) ? M1[j] : M2[j];

        // Sort. Отсортировать массив с результатами указанным методом
        sort(M, M2);

        prev = M2[0];
        // no pragma because: (1) array is already sorted and (2) just looking for first non-zero minimum
        for (size_t j = 1; j < M && prev == 0.0; ++j)
            prev = M2[j];

        // Reduce. Сумма синусов элементов M2, у которых при делении на минимальное ненулевое целая часть четная
        X = 0.0;
        # pragma omp parallel for default(none) shared(M2, M, prev) reduction(+:X)
        for (size_t j = 0; j < M; ++j)
            X += is_even(M2[j] / prev) ? sin(M2[j]) : 0.0;
        printf("X = %lf\n", X);
	}
    T2 = omp_get_wtime(); // запомнить текущее время T2

    delta_ms = (long)((T2 - T1) * 1000.0);
    printf("N=%d. Milliseconds passed: %ld\n", N, delta_ms);

freeMs:
    free(M2);
    free(M1);
exit:
    return ret;
}


static double random_double_r(double a, double b, unsigned* seedp)
{
    double number = 0.0;
    #pragma omp critical
    number = (double)rand_r(seedp);
    return a + number / (((double) RAND_MAX) / (b - a));
}

static int generate_random_uniform_array(size_t size, double array[size],
                                         double a, double b, unsigned seed)
{
    if (!array) return EINVAL;

    #pragma omp parallel for default(none) shared(seed, a, b, size, array)
    for (size_t i = 0; i < size; ++i)
        array[i] = random_double_r(a, b, &seed);
    return 0;
}
