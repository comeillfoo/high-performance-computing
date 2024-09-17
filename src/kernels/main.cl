__kernel void filter_fold(double min, unsigned long M, __global double* M2,
                          __global double* X)
{
    __local double acc;
    acc = 0.0;
    for (unsigned long i = 0; i < M; ++i)
        acc += convert_double(1 - (as_long(M2[i] / min) % 2)) * sin(M2[i]);
    *X = acc;
}
