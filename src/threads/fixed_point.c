#include <stdint.h>

#define q 16
#define f (1 << q)

typedef int fixed_point;

fixed_point int_to_fixed(int x) { //FP_CONST
    return (int)x * f;
}

int fixed_to_int_floor(fixed_point x) //FP_INT_PART
{
    return x / f;
}

int fixed_to_int_round(fixed_point x) { //FP_ROUND
    if (x >= 0)
        return (x + (f/2)) / f;
    else
        return (x - (f/2)) / f;
}

fixed_point fixed_add(fixed_point x, fixed_point y) {//FP_ADD
    return x + y; // Add the two fixed-point numbers
}

fixed_point fixed_subtract(fixed_point x, fixed_point y) { //FP_SUB
    return x - y; // Subtract the second fixed-point number from the first one
}

fixed_point int_fixed_add(fixed_point x, int n) //FP_ADD_MIX
{
    return x + (n * f);
}

fixed_point int_fixed_sub(int n, fixed_point x) //FP_SUB_MIX
{
    return x - (n * f);
}

fixed_point fixed_multiply(fixed_point x, fixed_point y) { //FP_MULT
    // Multiply the two fixed-point numbers and divide the result by f to get the fixed-point representation
    return (int)(((int64_t)x) * y / f);
}

fixed_point fixed_divide(fixed_point x, fixed_point y) { //FP_DIV
    // Multiply the first fixed-point number by f and divide the result by the second fixed-point number to get the fixed-point representation
    return (int)(((int64_t)x) * f / y);
}

fixed_point int_fixed_mul(fixed_point x, int n) //FP_MULT_MIX
{
    return x * n;
}

fixed_point int_fixed_div(fixed_point x, int n) //FP_DIV_MIX
{
    return x / n;
}
