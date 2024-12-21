#include "matrix.h"
#include <stdio.h>


int double_matrix_create(size_t rows, size_t cols, enum matrix_type type,
                         struct matrix* matp)
{
    if (!matp) return -1;
    matp->rows = rows;
    matp->cols = cols;
    matp->type = type;

    switch (matp->type) {
    case MT_VECTOR:
        matp->as_vector = malloc(rows * cols * sizeof(double));
        if (!matp->as_vector) return -1;
        break;
    case MT_TABLE:
        matp->as_table = malloc(rows * sizeof(double*));
        if (!matp->as_table) return -1;
        for (size_t i = 0; i < rows; ++i) {
            matp->as_table[i] = malloc(cols * sizeof(double));
            if (matp->as_table[i]) continue;
            for (size_t j = 0; j < i; ++j)
                free(matp->as_table[j]);
            free(matp->as_table);
            return -1;
        }
        break;
    default:
        break;
    }
    return 0;
}

void double_matrix_destroy(struct matrix mat)
{
    switch (mat.type) {
    case MT_VECTOR:
        free(mat.as_vector);
        break;
    case MT_TABLE:
        for (size_t i = 0; i < mat.rows; ++i)
            free(mat.as_table[i]);
        free(mat.as_table);
        break;
    default:
        break;
    }
}

int double_matrix_get(struct matrix* matp, size_t row, size_t col,
                      double* valup)
{
    if (!valup || !matp) return -1;
    if (matp->rows <= row || matp->cols <= col)
        return -1;

    switch (matp->type) {
    case MT_VECTOR:
        *valup = matp->as_vector[matp->cols * row + col];
        break;
    case MT_TABLE:
        *valup = matp->as_table[row][col];
        break;
    default:
        break;
    }
    return 0;
}

int double_matrix_set(struct matrix* matp, size_t row, size_t col,
                      double value)
{
    if (matp->rows <= row || matp->cols <= col)
        return -1;
    switch (matp->type) {
    case MT_VECTOR:
        matp->as_vector[matp->cols * row + col] = value;
        break;
    case MT_TABLE:
        matp->as_table[row][col] = value;
        break;
    default:
        break;
    }
    return 0;
}

double* double_matrix_get_row_mut(struct matrix* matp, size_t row)
{
    if (!matp || matp->rows <= row) return NULL;
    switch (matp->type) {
    case MT_VECTOR:
        return &matp->as_vector[matp->cols * row];
    case MT_TABLE:
        return matp->as_table[row];
    default:
        return NULL;
    }
    return NULL;
}

#ifdef USE_OPENCL
int oclw_async_write_matrix(struct matrix* matp, cl_command_queue queue,
                            cl_mem memobj, size_t rows, cl_event events[rows])
{
    int ret = 0;
    if (!matp) return -1;
    const size_t row_size = matp->cols * sizeof(double);
    for (size_t i = 0; i < rows; ++i) {
        size_t off = sizeof(double) * matp->cols * i;
        void* row = (void*)double_matrix_get_row_mut(matp, i);
        if (!row) {
            ret = -1;
            break;
        }
        ret = oclw_async_write_memobj(queue, memobj, off, row_size, row,
                                      &events[i]);
        if (ret) break;
    }
    return ret;
}

int oclw_async_read_matrix(struct matrix* matp, cl_command_queue queue,
                           cl_mem memobj, size_t rows, cl_event revents[rows],
                           cl_event* ceventp)
{
    int ret = 0;
    if (!matp) return -1;
    const size_t row_size = matp->cols * sizeof(double);
    for (size_t i = 0; i < rows; ++i) {
        size_t off = sizeof(double) * matp->cols * i;
        void* row = double_matrix_get_row_mut(matp, i);
        if (!row) {
            ret = -1;
            break;
        }
        ret = oclw_async_read_memobj_after(queue, memobj, off, row_size,
                                           row, 1, ceventp, &revents[i]);
        if (ret) break;
    }
    return ret;
}
#endif

int double_matrix_debug(struct matrix* matp)
{
    int ret = 0;
    if (!matp) return -1;
    for (size_t i = 0; i < matp->rows; ++i) {
        for (size_t j = 0; j < matp->cols; ++j) {
            double value = 0.0;
            ret = double_matrix_get(matp, i, j, &value);
            if (ret) return -1;
            printf(" %lf", value);
        }
        printf("\n");
    }
    return ret;
}
