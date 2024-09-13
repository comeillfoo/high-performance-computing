#include <math.h>
#include "mappers.h"


double map_sqrt_exp(double number)
{
    return exp(sqrt(number));
}

double map_abs_ctg(double number)
{
    return fabs(1.0 / tan(number));
}
