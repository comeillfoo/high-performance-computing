#pragma OPENCL EXTENSION cl_khr_fp64 : enable

kernel void filter_fold(double min, unsigned long M, constant double* M2,
                        global double* X)
{
    private double acc;
    acc = 0.0;
    for (unsigned long i = 0; i < M; ++i)
        acc += convert_double(1 - (convert_uint(M2[i] / min) % 2)) * sin(M2[i]);
    *X = acc;
}


kernel void map_sqrt_exp(unsigned long N, global double* M1)
{
    for (unsigned long i = 0; i < N; ++i)
        M1[i] = exp(sqrt(M1[i]));
}


kernel void map_abs_ctg(unsigned long M, global double* Mt, global double* M2)
{
    for (unsigned long i = 0; i < M; ++i)
        M2[i] = fabs(1.0 / tan(M2[i] + Mt[i]));
}

// source: https://math.stackexchange.com/a/67495
kernel void select_max(unsigned long M, global double* M1, global double* M2)
{
    for (unsigned long i = 0; i < M; ++i)
        M2[i] = (M2[i] + M1[i] + fabs(M2[i] - M1[i])) / 2;
}
