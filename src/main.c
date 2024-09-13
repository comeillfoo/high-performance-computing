#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
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

static inline bool is_even(double number) {
    return !(((uint_least64_t) number) % 2);
}

static int fill_random_uniform_array(size_t size, double array[size],
                                     double a, double b, unsigned seed);

static int args_parse(int argc, char* argv[], int* Np) {
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

int main(int argc, char* argv[]) {
    const double A = 450.0; // А = Ф * И * О
    int N, ret = 0;
    double prev, curr, X;
    size_t i, size;
    struct timeval T1, T2;
    long delta_ms;

    ret = args_parse(argc, argv, &N); // N равен первому параметру командной строки
    if (ret)
        goto exit;

    double* M1 = malloc(N * sizeof(double));
    size = N / 2;
    double* M2 = malloc(size * sizeof(double));

    gettimeofday(&T1, NULL); // запомнить текущее время T1
    for (i = 0; i < 100; ++i) { // 100 экспериментов
        // Generate. Заполнить массив исходных данных размером N
        fill_random_uniform_array(N, M1, 1.0, A, i);
        fill_random_uniform_array(size, M2, A, 10.0 * A, i);

        // Map. Решить поставленную задачу, заполнить массив с результатами
        #pragma omp parallel for default(none) shared(N, M1)
        for (size_t j = 0; j < N; ++j)
            M1[j] = map_sqrt_exp(M1[j]);

        prev = M2[0];
        // no pragmas because: of read/write dependencies
        for (size_t j = 1; j < size; ++j) {
            curr = map_abs_ctg(M2[j] + prev);
            prev = M2[j];
            M2[j] = curr;
        }

        #pragma omp parallel for default(none) shared(M1, M2, size)
        for (size_t j = 0; j < size; ++j)
            M2[j] = (M1[j] > M2[j]) ? M1[j] : M2[j];

        // Sort. Отсортировать массив с результатами указанным методом
        gnome_sort(size, M2);

        // Reduce. Сумма синусов элементов M2, у которых при делении на минимальное ненулевое целая часть четная
        prev = M2[0];
        // no pragma because: (1) array is already sorted and (2) just looking for first non-zero minimum
        for (size_t j = 1; j < size && prev == 0.0; ++j)
            prev = M2[j];

        X = 0.0;
        # pragma omp parallel for default(none) shared(M2, size, prev, X)
        for (size_t j = 0; j < size; ++j) {
            double x = is_even(M2[j] / prev) ? sin(M2[j]) : 0.0;
            #pragma omp atomic
            X += x;
        }
        printf("X = %lf\n", X);
	}
    gettimeofday(&T2, NULL); // запомнить текущее время T2
    free(M2);
    free(M1);

    delta_ms = (T2.tv_sec - T1.tv_sec) * 1000
        + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("N=%d. Milliseconds passed: %ld\n", N, delta_ms);

exit:
    return ret;
}


static double random_double_r(double a, double b, unsigned* seedp) {
    double number = 0.0;
    #pragma omp critical
    number = (double)rand_r(seedp);
    return a + number / (((double) RAND_MAX) / (b - a));
}

static int fill_random_uniform_array(size_t size, double array[size],
                                     double a, double b, unsigned seed) {
    if (!array) return EINVAL;

    #pragma omp parallel for default(none) shared(seed, a, b, size, array)
    for (size_t i = 0; i < size; ++i)
        array[i] = random_double_r(a, b, &seed);
    return 0;
}
