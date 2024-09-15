#include "futils.h"

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