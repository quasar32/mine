#ifndef MISC_H
#define MISC_H

#include <stddef.h>
#include <math.h>
#include <cglm/struct.h>

static inline int imin(int a, int b) {
    return a < b ? a : b;
}

static inline int imax(int a, int b) {
    return a > b ? a : b;
}

static inline float fclampf(float v, float l, float h) {
    return fminf(fmaxf(v, l), h);
}

static inline float iclamp(float v, float l, float h) {
    return imin(imax(v, l), h);
}

static inline int face_dir(int face) {
    return face % 2 * 2 - 1;
}

static inline ivec3s ivec3_add_to_axis(ivec3s a, int b, int i) {
    a.raw[i] += b;
    return a;
}

void die(const char *fmt, ...);
void *xmalloc(size_t sz);

#endif
