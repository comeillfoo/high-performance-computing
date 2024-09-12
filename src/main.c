#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>


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

static int fill_random_matrix(float* matrix, size_t size, float A);

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
    const float A = 450.0f; // А = Ф * И * О
    int N, ret = 0;
    size_t i;
    struct timeval T1, T2;
    long delta_ms;

    ret = args_parse(argc, argv, &N); // N равен первому параметру командной строки
    if (ret)
        goto exit;

    float* M1 = malloc(N * sizeof(float));
    float* M2 = malloc((N / 2) * sizeof(float));

    gettimeofday(&T1, NULL); // запомнить текущее время T1
    for (i = 0; i < 100; ++i) {       // 100 экспериментов
        fill_random_matrix(M1, N, A); // Заполнить массив исходных данных размером N
                  // Решить поставленную задачу, заполнить массив с результатами
                  // Отсортировать массив с результатами указанным методом
	}
    gettimeofday(&T2, NULL); // запомнить текущее время T2
    free(M1);
    free(M2);

    delta_ms = (T2.tv_sec - T1.tv_sec) * 1000
        + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms);

exit:
    return ret;
}


static float random_float_r(float a, float b, unsigned* seedp) {
    return a + ((float)rand_r(seedp)) / (((float) RAND_MAX) / (b - a));
}

static int fill_random_matrix(float* matrix, size_t size, float A) {
    unsigned seed = time(NULL);
    if (!matrix) return EINVAL;

    for (size_t i = 0; i < size; ++i)
        matrix[i] = random_float_r(1.0, A, &seed);
    return 0;
}
