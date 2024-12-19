#pragma OPENCL EXTENSION cl_khr_fp64 : enable

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

kernel void filter_fold(double min, unsigned long M, constant double* M2,
                        global double* X)
{
    private double acc;
    acc = 0.0;
    for (unsigned long i = 0; i < M; ++i)
        acc += convert_double(1 - (convert_uint(M2[i] / min) % 2)) * sin(M2[i]);
    *X = acc;
}

// source: https://math.stackexchange.com/a/67495
kernel void select_max(unsigned long M, global double* M1, global double* M2)
{
    for (unsigned long i = 0; i < M; ++i)
        M2[i] = (M2[i] + M1[i] + fabs(M2[i] - M1[i])) / 2;
}
