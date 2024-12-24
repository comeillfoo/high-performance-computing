#pragma OPENCL EXTENSION cl_khr_fp64 : enable

// ===============
// mappers kernels
// ===============
kernel void apply_coth_sqrt(global double* vector)
{
    unsigned long gid = get_global_id(0);
    vector[gid] = 1.0 / tanh(sqrt(vector[gid]));
}

kernel void combine_abs_sin_sum(global const double* source, global double* target)
{
    unsigned long gid = get_global_id(0);
    target[gid] = fabs(sin(target[gid] + source[gid]));
}

kernel void shift_matrices(global const double* source, global double* target,
                           unsigned long shift)
{
    unsigned long i = get_global_id(0);
    unsigned long j = get_global_id(1);
    unsigned long rows = get_global_size(0);
    unsigned long cols = get_global_size(1);
    target[i * cols + (j + shift) % cols] = source[i * cols + j];
}

// ===============
// mergers kernels
// ===============
kernel void merge_by_pow(unsigned long src_cols, global const double* source,
                         unsigned long dst_cols, global double* target)
{
    unsigned long i = get_global_id(0);
    unsigned long j = get_global_id(1);
    unsigned long si = i * src_cols + j;
    unsigned long ti = i * dst_cols + j;
    target[ti] = pow(source[si], target[ti]);
}

// ===============
// multipliers kernels
// ===============
kernel void multiply(const unsigned long Acols, global const double* A,
                     global const double* B, global double* C)
{
    unsigned long i = get_global_id(0);
    unsigned long j = get_global_id(1);
    unsigned long Bcols = get_global_size(1);
    private double tmp = 0.0;
    for (unsigned long k = 0; k < Acols; ++k)
        tmp += A[i * Acols + k] * B[k * Bcols + j];
    C[i * Bcols + j] = tmp;
    // printf("OCL[%zu, %zu]: %F\n", i, j, C[i * Bcols + j]);
}

// ===============
// sorts kernels
// ===============
kernel void selection_sort_only_row(unsigned long size, global double* matrix)
{
    unsigned long row = get_global_id(0);
    for (unsigned long i = 0; i < size - 1; ++i) {
        private unsigned long mini = i;
        private double tmp = matrix[row * size + i];
        for (unsigned long j = i + 1; j < size; ++j)
            if (matrix[row * size + j] < matrix[row * size + mini])
                mini = j;
        matrix[row * size + i] = matrix[row * size + mini];
        matrix[row * size + mini] = tmp;
    }
}

void selection_sort_half(unsigned long size, local double* halves)
{
    for (unsigned long i = 0; i < size - 1; ++i) {
        private unsigned long mini = i;
        private double tmp = halves[i];
        for (unsigned long j = i + 1; j < size; ++j)
            if (halves[j] < halves[mini])
                mini = j;
        halves[i] = halves[mini];
        halves[mini] = tmp;
    }
}

kernel void selection_sort_halves(unsigned long size, global double* matrix,
                                  local double* halves)
{
    unsigned long row = get_global_id(0);
    unsigned long half_nr = get_local_id(0);
    unsigned long first = size / 2;
    unsigned long rest = size - first;
    if (half_nr < 1) {
        // printf("OCL[%zu:%zu]: sorts [%zu; %zu]\n", row, half_nr, 0UL, first - 1);
        for (unsigned long i = 0; i < first; ++i)
            halves[i] = matrix[row * size + i];
        selection_sort_half(first, halves);
    } else {
        // printf("OCL[%zu:%zu]: sorts [%zu; %zu]\n", row, half_nr, first, size - 1);
        for (unsigned long i = 0; i < rest; ++i)
            halves[i + first] = matrix[row * size + i + first];
        selection_sort_half(rest, halves + first);
    }

    barrier(CLK_LOCAL_MEM_FENCE);

    if (half_nr < 1) {
        private unsigned int i = 0;
        private unsigned int j = first;
        private unsigned int k = 0;
        while (i < first && j < size) {
            if (halves[i] < halves[j]) {
                matrix[row * size + k] = halves[i];
                ++i; ++k;
                continue;
            }
            matrix[row * size + k] = halves[j];
            ++j; ++k;
        }

        while (i < first) {
            matrix[row * size + k] = halves[i];
            ++i; ++k;
        }

        while (j < size) {
            matrix[row * size + k] = halves[j];
            ++j; ++k;
        }
    }
}

// ===============
// reducers kernels
// ===============
kernel void reduce(unsigned long size, global const double* matrix, global double* psums)
{
    unsigned long i = get_global_id(0);
    private double minimum = 0.0;
    for (unsigned long j = 0; j < size; ++j) {
        minimum = matrix[i * size + j];
        if (minimum > 0.0) break;
    }
    if (minimum == 0.0) return;

    private double psum = 0.0;
    for (unsigned long j = 0; j < size; ++j) {
        double value = matrix[i * size + j];
        psum += convert_double(1 - (convert_uint(value / minimum) % 2)) * sin(value);
    }
    psums[i] = psum;
    // printf("OCL[%zu]: %F\n", i, psums[i]);
}

// source: https://math.stackexchange.com/a/67495
kernel void select_max(unsigned long M, global double* M1, global double* M2)
{
    for (unsigned long i = 0; i < M; ++i)
        M2[i] = (M2[i] + M1[i] + fabs(M2[i] - M1[i])) / 2;
}
