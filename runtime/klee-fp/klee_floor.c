#include "klee/klee.h"
#include "klee_floor.h"
#include "math.h"

float klee_floorf(float x) {
    int sign = signbit(x);
    x = klee_abs_float(x);
    if (klee_rintf(x) > x) {
        return sign * (klee_rintf(x) - 1);
    } else {
        return sign * klee_rintf(x);
    }
}

double klee_floor(double x) {
    int sign = signbit(x);
    x = klee_abs_double(x);
    if (klee_rint(x) > x) {
        return sign * (klee_rint(x) - 1);
    } else {
        return sign * klee_rint(x);
    }
}

long double klee_floorl(long double x) {
    int sign = signbit(x);
    x = klee_abs_long_double(x);
    if (klee_rintl(x) > x) {
        return sign * (klee_rintl(x) - 1);
    } else {
        return sign * klee_rintl(x);
    }
}
