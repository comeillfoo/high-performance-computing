#include <stdint.h>

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

static void _parallel_gnome_sort(size_t size, double array[size])
{
    const size_t lsize = size / 2;
    const size_t rsize = size - lsize;
    size_t i = 0, j = 0, k = 0;
    double left[lsize], right[rsize];
#ifdef _OPENMP
    _omp_gnome_sort(size, array, lsize, left, rsize, right);
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
#endif

void sort(size_t size, double array[size])
{
#ifdef _OPENMP
    _parallel_gnome_sort(size, array);
#else
    _gnome_sort(size, array);
#endif
}
