#ifndef _GENERATORS_H_
#define _GENERATORS_H_

#include <stdlib.h>

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

int just_generate_random_matrix(size_t rows, size_t cols, double** matrix,
								double a, double b, unsigned seed);
#endif // _GENERATORS_H_