#ifndef MISC_H
#define MISC_H

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, l, h) (MIN(MAX((v), (l)), (h)))

#define V3SS(a, v) (a[(v)[0]][(v)[1]][(v)[2]])

static inline int face_dir(int face) {
    return face % 2 * 2 - 1;
}

void die(const char *fmt, ...);

#endif
