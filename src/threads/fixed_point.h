#include <stdint.h>

typedef int fixed_point;

fixed_point int_to_fixed(int x);

int fixed_to_int_floor(fixed_point x) ;

int fixed_to_int_round(fixed_point x) ;

fixed_point fixed_add(fixed_point x, fixed_point y);

fixed_point fixed_subtract(fixed_point x, fixed_point y);

fixed_point int_fixed_add(fixed_point x, int n); 

fixed_point int_fixed_sub(int n, fixed_point x);

fixed_point fixed_multiply(fixed_point x, fixed_point y);

fixed_point fixed_divide(fixed_point x, fixed_point y) ;

fixed_point int_fixed_mul(fixed_point x, int n); 

fixed_point int_fixed_div(fixed_point x, int n);

