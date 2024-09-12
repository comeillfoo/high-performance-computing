#include <stdint.h>

#include "sorts.h"

static inline void swap(uint_least64_t* restrict a,
                        uint_least64_t* restrict b) {
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

void gnome_sort(size_t size, double array[size]) {
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
