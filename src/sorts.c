#include <stdint.h>
#include <omp.h>

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

static void _omp_gnome_sort(size_t size, double array[size])
{
    const size_t left_size = size / 2;
    const size_t right_size = size - left_size;
    size_t i = 0, j = 0, k = 0;
    double left[left_size], right[right_size];
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            for (i = 0; i < left_size; ++i)
                left[i] = array[i];
            _gnome_sort(left_size, left);
        }

        #pragma omp section
        {
            for (j = 0; j < right_size; ++j)
                right[j] = array[j + left_size];
            _gnome_sort(right_size, right);
        }
    }
    i = 0;
    j = 0;
    while (i < left_size && j < right_size) {
        if (left[i] < right[j]) {
            array[k] = left[i];
            ++i; ++k;
            continue;
        }
        array[k] = right[j];
        ++j; ++k;
    }

    while (i < left_size) {
        array[k] = left[i];
        ++i; ++k;
    }

    while (j < right_size) {
        array[k] = right[j];
        ++j; ++k;
    }
}

void sort(size_t size, double array[size])
{
    _omp_gnome_sort(size, array);
}
