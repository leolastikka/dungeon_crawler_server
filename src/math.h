#ifndef MATH_H
#define MATH_H

typedef struct {
    float x;
    float y;
} vec_2d_t;

vec_2d_t vec_2d_mult(vec_2d_t vec, float scalar);
vec_2d_t vec_2d_add(vec_2d_t vec, vec_2d_t add);

#endif