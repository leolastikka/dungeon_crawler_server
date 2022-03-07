#include "math.h"

vec_2d_t vec_2d_mult(vec_2d_t vec, float scalar) {
    vec_2d_t res;
    res.x = vec.x * scalar;
    res.y = vec.y * scalar;
    return res;
}

vec_2d_t vec_2d_add(vec_2d_t first, vec_2d_t second) {
    vec_2d_t res;
    res.x = first.x + second.x;
    res.y = first.y + second.y;
    return res;
}