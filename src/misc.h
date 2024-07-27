#ifndef MISC_H
#define MISC_H

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, l, h) (MIN(MAX((v), (l)), (h)))

static inline int face_dir(int face) {
    return face % 2 * 2 - 1;
}

void die(const char *fmt, ...);

#endif
