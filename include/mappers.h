#ifndef _MAPPERS_H_
#define _MAPPERS_H_

typedef double (*mapper)(double);

double map_sqrt_exp(double number);
double map_abs_ctg(double number);

#ifdef _PTHREAD_H
int parallel_map_array(size_t size, double array[size], mapper fn,
                       size_t threads);
#endif

#endif // _MAPPERS_H_
