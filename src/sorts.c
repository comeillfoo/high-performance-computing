#include <stdint.h>

#include "sorts.h"

static inline void swap(uint_least64_t* restrict a,
                        uint_least64_t* restrict b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

#ifdef USE_PTHREAD
struct _sort_routine_args
{
    size_t size;
    double* sorted;
    double* array;
};

static void* _gnome_sort_routine(void* args)
{
    struct _sort_routine_args* sort_args = (struct _sort_routine_args*)args;
    for (size_t i = 0; i < sort_args->size; ++i)
        sort_args->sorted[i] = sort_args->array[i];
    gnome_sort(sort_args->size, sort_args->sorted);
    return (void*) 0;
}

static void _pthread_gnome_sort(size_t size, double array[size],
                                size_t lsize, double left[lsize],
                                size_t rsize, double right[rsize])
{
    pthread_t threads_id[2];
    struct _sort_routine_args threads_args[] = {
        { lsize, left, array },
        { rsize, right, array + lsize }
    };
    for (size_t i = 0; i < 2; ++i) {
        pthread_create(&threads_id[i], NULL, _gnome_sort_routine,
                       (void*)(&threads_args[i]));
    }
    for (size_t i = 0; i < 2; ++i)
        pthread_join(threads_id[i], NULL);
}
#elif defined(_OPENMP)
static int selection_sort(size_t size, double* array)
{
    if (!array) return -1;
    for (size_t i = 0; i < size - 1; ++i) {
        size_t mini = i;
        for (size_t j = i + 1; j < size; ++j)
            if (array[j] < array[mini])
                mini = j;

        swap((uint_least64_t*)(array + i), (uint_least64_t*)(array + mini));
    }
    return 0;
}

static void parallel_selection_sort(double* array, size_t lsize, double left[lsize],
                                   size_t rsize, double right[rsize])
{
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            for (size_t i = 0; i < lsize; ++i)
                left[i] = array[i];
            selection_sort(lsize, left);
        }

        #pragma omp section
        {
            for (size_t j = 0; j < rsize; ++j)
                right[j] = array[j + lsize];
            selection_sort(rsize, right);
        }
    }
}

static void parallel_merge_selection_sort(size_t size, double* array)
{
    const size_t lsize = size / 2;
    const size_t rsize = size - lsize;
    size_t i = 0, j = 0, k = 0;
    double left[lsize], right[rsize];
    if (!array) return;
    parallel_selection_sort(array, lsize, left, rsize, right);

    i = 0;
    j = 0;
    while (i < lsize && j < rsize) {
        if (left[i] < right[j]) {
            array[k] = left[i];
            ++i; ++k;
            continue;
        }
        array[k] = right[j];
        ++j; ++k;
    }

    while (i < lsize) {
        array[k] = left[i];
        ++i; ++k;
    }

    while (j < rsize) {
        array[k] = right[j];
        ++j; ++k;
    }
}

int sort_rows(struct matrix* matp)
{
    if (!matp) return -1;
    #pragma omp parallel for default(none) shared(matp)
    for (size_t i = 0; i < matp->rows; ++i) {
        switch (matp->type) {
        case MT_VECTOR:
            parallel_merge_selection_sort(matp->cols,
                matp->as_vector + (i * matp->cols));
            break;
        case MT_TABLE:
            parallel_merge_selection_sort(matp->cols, matp->as_table[i]);
            break;
        default:
            break;
        }
    }
    return 0;
}
#else
// https://www.geeksforgeeks.org/selection-sort-algorithm-2/
static int selection_sort(size_t size, double* array)
{
    if (!array) return -1;
    for (size_t i = 0; i < size - 1; ++i) {
        size_t mini = i;
        for (size_t j = i + 1; j < size; ++j)
            if (array[j] < array[mini])
                mini = j;

        swap((uint_least64_t*)(array + i), (uint_least64_t*)(array + mini));
    }
    return 0;
}

int sort_rows(struct matrix* matp)
{
    int ret = 0;
    if (!matp) return -1;
    for (size_t i = 0; i < matp->rows; ++i) {
        switch (matp->type) {
        case MT_VECTOR:
            ret = selection_sort(matp->cols, matp->as_vector + (i * matp->cols));
            break;
        case MT_TABLE:
            ret = selection_sort(matp->cols, matp->as_table[i]);
            break;
        default:
            break;
        }
        if (ret) return ret;
    }
    return ret;
}
#endif

