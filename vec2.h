#ifndef e_vec2
#define e_vec2

typedef double *vec2_t;

vec2_t vec2_subtract(vec2_t vec, vec2_t vec2, vec2_t dest);
vec2_t vec2_add(vec2_t vec, vec2_t vec2, vec2_t dest);
vec2_t vec2_scale(vec2_t vec, double val, vec2_t dest);
double vec2_dot(vec2_t vec, vec2_t vec2);

#endif
