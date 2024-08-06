#ifndef MISC_H
#define MISC_H

#include <stddef.h>
#include <math.h>

static inline int imin(int a, int b) {
    return a < b ? a : b;
}

static inline int imax(int a, int b) {
    return a > b ? a : b;
}

static inline float fclampf(float v, float l, float h) {
    return fminf(fmaxf(v, l), h);
}

static inline int face_dir(int face) {
    return face % 2 * 2 - 1;
}

void die(const char *fmt, ...);
void *xmalloc(size_t sz);

#endif
