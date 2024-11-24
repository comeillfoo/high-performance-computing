#ifndef _FUTILS_H_
#define _FUTILS_H_

#include <stdio.h>

#ifdef _OPENMP
#include <omp.h>
#else
#include <sys/time.h>
#endif

struct tstamp {
#ifdef _OPENMP
    double seconds;
#else
    struct timeval tval;
#endif
};

int stamp_time(struct tstamp* tstamp);
long stamps_diff_ms(struct tstamp t1, struct tstamp t2);
ssize_t fget_size_verbose(FILE* stream);
int fclose_verbose(FILE* stream);

#endif // _FUTILS_H_
