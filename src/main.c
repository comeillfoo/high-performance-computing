#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>


static int usage(int argc, char* argv[]) {
    if (argc > 1)
        return 0;
    printf("Usage: %s N\n\n", argv[0]);
    printf("Arguments:\n    N    size of matrices\n");
    return EINVAL;
}

int main(int argc, char* argv[]) {
    int i, N, ret = 0;
    struct timeval T1, T2;
    long delta_ms;
    ret = usage(argc, argv);
    if (ret)
        goto exit;

    N = atoi(argv[1]); // N равен первому параметру командной строки

    gettimeofday(&T1, NULL); // запомнить текущее время T1
    for (i = 0; i < 100; ++i) { // 100 экспериментов
        srand(i); // инициализировать начальное значение ГСЧ
                  // Заполнить массив исходных данных размером N
                  // Решить поставленную задачу, заполнить массив с результатами
                  // Отсортировать массив с результатами указанным методом
	}
    gettimeofday(&T2, NULL); // запомнить текущее время T2

    delta_ms = (T2.tv_sec - T1.tv_sec) * 1000 +
    (T2.tv_usec - T1.tv_usec) / 1000;
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms);

exit:
    return ret;
}