__kernel void filter_fold(double min, unsigned long M, __local double* M2, __global double* X)
{
    *X = 0.0;
    for (unsigned long i = 0; i < M; ++i)
        *X += convert_double(1 - (as_long(M2[i] / min) % 2)) * sin(M2[i]);
}
