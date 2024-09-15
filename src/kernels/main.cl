// __kernel void filter_fold(double min, unsigned long M, __local double* M2, __global double* X)
// {
//     *X = 0.0;
//     for (unsigned long i = 0; i < M; ++i)
//         *X += convert_double(1 - (as_long(M2[i] / min) % 2)) * sin(M2[i]);
// }

__kernel void hello(__global char *string)
{
    string[0] = 'H';
    string[1] = 'e';
    string[2] = 'l';
    string[3] = 'l';
    string[4] = 'o';
    string[5] = ',';
    string[6] = ' ';
    string[7] = 'W';
    string[8] = 'o';
    string[9] = 'r';
    string[10] = 'l';
    string[11] = 'd';
    string[12] = '!';
    string[13] = '\0';
}
