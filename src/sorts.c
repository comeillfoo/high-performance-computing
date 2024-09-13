#include <stdint.h>
#include <pthread.h>

#include "sorts.h"

static inline void swap(uint_least64_t* restrict a,
                        uint_least64_t* restrict b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

static void _gnome_sort(size_t size, double array[size])
{
    size_t i = 0;
    while (i < size) {
        if (i == 0 || array[i] >= array[i - 1]) {
            ++i;
            continue;
        }
        swap((uint_least64_t*)(array + i),
             (uint_least64_t*)(array + (i - 1)));
        --i;
    }
}

#ifdef _OPENMP
static inline void _omp_gnome_sort(size_t size, double array[size],
                                   size_t lsize, double left[lsize],
                                   size_t rsize, double right[rsize])
{
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            for (size_t i = 0; i < lsize; ++i)
                left[i] = array[i];
            _gnome_sort(lsize, left);
        }

        #pragma omp section
        {
            for (size_t j = 0; j < rsize; ++j)
                right[j] = array[j + lsize];
            _gnome_sort(rsize, right);
        }
    }
}
#endif

#ifdef _PTHREAD_H
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
    _gnome_sort(sort_args->size, sort_args->sorted);
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
#endif

static void _parallel_gnome_sort(size_t size, double array[size])
{
    const size_t lsize = size / 2;
    const size_t rsize = size - lsize;
    size_t i = 0, j = 0, k = 0;
    double left[lsize], right[rsize];
#ifdef _OPENMP
    _omp_gnome_sort(size, array, lsize, left, rsize, right);
#elif defined(_PTHREAD_H)
    _pthread_gnome_sort(size, array, lsize, left, rsize, right);
#else
    #error Fatal bug on compiling: without libs should be sequential sort picked
#endif
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

void sort(size_t size, double array[size])
{
#if defined(_OPENMP) || defined(_PTHREAD_H)
    _parallel_gnome_sort(size, array);
#else
    _gnome_sort(size, array);
#endif
}
