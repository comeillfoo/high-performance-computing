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
