#ifndef _JUST_H_
#define _JUST_H_

#include <stdbool.h>

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

bool is_even(double number);
int generate_random_uniform_array(size_t size, double array[size],
                                  double a, double b, unsigned seed);

int alloc_double_matrix(double*** Mppp, size_t rows, size_t cols);
#endif // _JUST_H_