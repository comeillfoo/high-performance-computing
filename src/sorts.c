#include <stdint.h>

#include "sorts.h"

static inline void swap(uint_least64_t* restrict a,
                        uint_least64_t* restrict b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}

#ifndef USE_OPENCL
// https://www.geeksforgeeks.org/selection-sort-algorithm-2/
static int selection_sort(size_t size, double* array)
{
    if (!array) return -1;
    for (size_t i = 0; i < size - 1; ++i) {
        size_t mini = i;
        for (size_t j = i + 1; j < size; ++j)
            if (array[j] < array[mini])
                mini = j;

        swap((uint_least64_t*)(array + i), (uint_least64_t*)(array + mini));
    }
    return 0;
}
#endif

// #define USE_OPENCL
#ifdef USE_OPENCL
#include "oclw.h"

extern cl_context ocl_context;
extern cl_command_queue ocl_queue;

cl_kernel selection_sort_kern = NULL;

#ifdef PARALLEL_SORT_ONLY_ROWS
#define SELECTION_SORT_LOCAL_SIZE (1)
#else
#define SELECTION_SORT_LOCAL_SIZE (2)
#endif
int sort_rows(struct matrix* matp)
{
    int ret = 0;
    cl_mem ocl_mem = NULL;
    if (!matp) return -1;
    if (!matp->rows || !matp->cols) return 0;
    if (!ocl_context || !ocl_queue || !selection_sort_kern) return -1;

    cl_event wevents[matp->rows];
    cl_event cevent = NULL;
    cl_event revents[matp->rows];

    ret = oclw_create_memobj(ocl_context, CL_MEM_READ_WRITE, &ocl_mem, matp->rows
                             * matp->cols * sizeof(double), NULL);
    if (ret) goto exit;
    ret = oclw_set_kernel_arg(selection_sort_kern, 0, sizeof(matp->cols),
                              &matp->cols, "size");
    if (ret) goto free_memobj;
    ret = oclw_set_kernel_arg(selection_sort_kern, 1, sizeof(cl_mem), &ocl_mem,
                              "matrix");
    if (ret) goto free_memobj;
#ifndef PARALLEL_SORT_ONLY_ROWS
    ret = oclw_set_kernel_arg(selection_sort_kern, 2, sizeof(double) * matp->cols,
                              NULL, "halves");
    if (ret) goto free_memobj;
#endif
    // fill memory object with rows
    ret = oclw_async_write_matrix(matp, ocl_queue, ocl_mem, matp->rows, wevents);
    if (ret) goto free_memobj;

    // run task on memory object after writes
    size_t local_work_size = SELECTION_SORT_LOCAL_SIZE;
    ret = oclw_async_run_task_after(ocl_queue, selection_sort_kern, matp->rows *
                                    local_work_size, &local_work_size,
                                    matp->rows, wevents, &cevent);
    if (ret) goto free_memobj;

    // read results into original matrix after completion
    ret = oclw_async_read_matrix(matp, ocl_queue, ocl_mem, matp->rows, revents,
                                 &cevent);
    if (ret) goto free_memobj;

    // wait till everything is completed
    ret = oclw_wait_till_completion(matp->rows, revents);
free_memobj:
    ret |= oclw_destroy_memobj(ocl_mem);
exit:
    return ret;
}
#undef SELECTION_SORT_LOCAL_SIZE
#elif defined(USE_PTHREAD)
#include "ptpool.h"
extern struct ptpool* pool;

#ifndef PARALLEL_SORT_ONLY_ROWS
struct _halfsort_args
{
    size_t size;
    double* sorted;
    double* array;
};

static void* _halfsort_routine(void* args)
{
    struct _halfsort_args* targs = (struct _halfsort_args*)args;
    for (size_t i = 0; i < targs->size; ++i)
        targs->sorted[i] = targs->array[i];
    return (void*) ((intptr_t) selection_sort(targs->size, targs->sorted));
}

static int parallel_selection_sort(double* array, size_t lsize, double left[lsize],
                                   size_t rsize, double right[rsize])
{
    struct _halfsort_args tasks_args[] = {
        { lsize, left, array },
        { rsize, right, array + lsize }
    };
    struct ptpool* halfsort_pool = ptpool_create(2, pool->type);
    if (!halfsort_pool)
        return -1;

    for (size_t i = 0; i < 2; ++i) {
        if (!ptpool_enqueue_task(halfsort_pool, _halfsort_routine,
                                 (void*)(&tasks_args[i])))
            goto destroy;
    }

    ptpool_wait(halfsort_pool);
destroy:
    ptpool_destroy(halfsort_pool);
    return 0;
}

static int parallel_merge_selection_sort(size_t size, double* array)
{
    const size_t lsize = size / 2;
    const size_t rsize = size - lsize;
    size_t i = 0, j = 0, k = 0;
    double left[lsize], right[rsize];
    if (!array) return -1;
    if (parallel_selection_sort(array, lsize, left, rsize, right))
        return -1;

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
    return 0;
}
#endif

static int sort_cols(size_t size, double* array)
{
#ifdef PARALLEL_SORT_ONLY_ROWS
    return selection_sort(size, array);
#else
    return parallel_merge_selection_sort(size, array);
#endif
}

struct _rowsort_args {
    size_t row;
    struct matrix* matp;
};

static void* _rowsort_routine(void* args)
{
    int ret = 0;
    struct _rowsort_args* targs = (struct _rowsort_args*)args;
    struct matrix* matp = targs->matp;
    ret = sort_cols(matp->cols, double_matrix_get_row_mut(matp, targs->row));
    return (void*) ((intptr_t) ret);
}

int sort_rows(struct matrix* matp)
{
    if (!matp) return -1;
    struct _rowsort_args tasks_args[matp->rows];
    for (size_t i = 0; i < matp->rows; ++i) {
        tasks_args[i].matp = matp;
        tasks_args[i].row = i;
        if (!ptpool_enqueue_task(pool, _rowsort_routine, &tasks_args[i]))
            return -1;
    }

    ptpool_wait(pool);
    return 0;
}
#elif defined(_OPENMP)
#ifndef PARALLEL_SORT_ONLY_ROWS
static void parallel_selection_sort(double* array, size_t lsize, double left[lsize],
                                   size_t rsize, double right[rsize])
{
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            for (size_t i = 0; i < lsize; ++i)
                left[i] = array[i];
            selection_sort(lsize, left);
        }

        #pragma omp section
        {
            for (size_t j = 0; j < rsize; ++j)
                right[j] = array[j + lsize];
            selection_sort(rsize, right);
        }
    }
}

static void parallel_merge_selection_sort(size_t size, double* array)
{
    const size_t lsize = size / 2;
    const size_t rsize = size - lsize;
    size_t i = 0, j = 0, k = 0;
    double left[lsize], right[rsize];
    if (!array) return;
    parallel_selection_sort(array, lsize, left, rsize, right);

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

static void sort_cols(size_t size, double* array)
{
#ifdef PARALLEL_SORT_ONLY_ROWS
    selection_sort(size, array);
#else
    parallel_merge_selection_sort(size, array);
#endif
}

int sort_rows(struct matrix* matp)
{
    if (!matp) return -1;
    #pragma omp parallel for default(none) shared(matp) schedule(runtime)
    for (size_t i = 0; i < matp->rows; ++i)
        sort_cols(matp->cols, double_matrix_get_row_mut(matp, i));
    return 0;
}
#else
int sort_rows(struct matrix* matp)
{
    int ret = 0;
    if (!matp) return -1;
    for (size_t i = 0; i < matp->rows; ++i) {
        ret = selection_sort(matp->cols, double_matrix_get_row_mut(matp, i));
        if (ret) return ret;
    }
    return ret;
}
#endif

