#include "futils.h"


int stamp_time(struct tstamp* t)
{
    if (!t) return -1;
#ifdef _OPENMP
    t->seconds = omp_get_wtime();
    return 0;
#else
    return gettimeofday(&(t->tval), NULL);
#endif
}

long stamps_diff_ms(struct tstamp t1, struct tstamp t2)
{
#ifdef _OPENMP
    return (long)((t2.seconds - t1.seconds) * 1000.0);
#else
    return (t2.tval.tv_sec - t1.tval.tv_sec) * 1000.0
        + (t2.tval.tv_usec - t1.tval.tv_usec) / 1000.0;
#endif
}

ssize_t fget_size_verbose(FILE* stream)
{
    ssize_t ret = -1;
    if (fseek(stream, 0L, SEEK_END) == -1) {
        perror("Unable to seek stream to the end");
        goto error;
    }
    ret = ftell(stream);
    if (ret == -1) {
        perror("Unable to get current position of binary");
        goto error;
    }
    if (fseek(stream, 0L, SEEK_SET) == -1) {
        perror("Unable to rewind stream back to beginning");
        goto error;
    }
    return ret;
error:
    fclose_verbose(stream);
    return -1;
}

int fclose_verbose(FILE* stream)
{
    if (fclose(stream) == EOF) {
        perror("Unable to close file");
        return 1;
    }
    return 0;
}
