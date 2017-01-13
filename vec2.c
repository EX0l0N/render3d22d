#include <stdlib.h>
#include <math.h>

#include "vec2.h"

vec2_t vec2_subtract(vec2_t vec, vec2_t vec2, vec2_t dest) {
	if (!dest || vec == dest) {
		vec[0] -= vec2[0];
		vec[1] -= vec2[1];
		return vec;
	}

	dest[0] = vec[0] - vec2[0];
	dest[1] = vec[1] - vec2[1];
	return dest;
}

vec2_t vec2_add(vec2_t vec, vec2_t vec2, vec2_t dest) {
    if (!dest || vec == dest) {
        vec[0] += vec2[0];
        vec[1] += vec2[1];
        return vec;
    }

    dest[0] = vec[0] + vec2[0];
    dest[1] = vec[1] + vec2[1];
    
    return dest;
}

vec2_t vec2_scale(vec2_t vec, double val, vec2_t dest) {
    if (!dest || vec == dest) {
        vec[0] *= val;
        vec[1] *= val;
        return vec;
    }

    dest[0] = vec[0] * val;
    dest[1] = vec[1] * val;
    return dest;
}

double vec2_dot(vec2_t vec, vec2_t vec2) {
	return vec[0] * vec2[0] + vec[1] * vec2[1];
}
