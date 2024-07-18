#include "map.h"
#include "misc.h"

static struct bounds map_bounds = {
    {0, 0, 0},
    {NX_CHUNK * CHUNK_LEN, NY_CHUNK * CHUNK_LEN, NZ_CHUNK * CHUNK_LEN}
};

static struct bounds chunk_bounds = {
    {0, 0, 0},
    {CHUNK_LEN, CHUNK_LEN, CHUNK_LEN},
};

struct chunk map[NX_CHUNK][NY_CHUNK][NZ_CHUNK];
struct chunk *dirty_chunks[N_CHUNKS];
int n_dirty_chunks;

static int inbounds(ivec3 pos, struct bounds *bounds) {
    return pos[0] >= bounds->min[0] && pos[0] < bounds->max[0] &&
           pos[1] >= bounds->min[1] && pos[1] < bounds->max[1] &&
           pos[2] >= bounds->min[2] && pos[2] < bounds->max[2];
}

static struct chunk *get_chunk(ivec3 v) {
    return &map[v[0] / CHUNK_LEN]
               [v[1] / CHUNK_LEN]
               [v[2] / CHUNK_LEN];
}

int in_chunk(ivec3 pos) {
    return inbounds(pos, &chunk_bounds);
}

int in_map(ivec3 pos) {
    return inbounds(pos, &map_bounds);
}

static void chunk_offset(ivec3 src, ivec3 dst) {
    int i;

    for (i = 0; i < 3; i++) {
        dst[i] = src[i] % CHUNK_LEN;
    }
}

uint8_t *get_block(ivec3 pos) {
    struct chunk *chunk;
    ivec3 off;

    if (!in_map(pos)) {
        return NULL;
    }
    chunk = get_chunk(pos);
    chunk_offset(pos, off);
    return &V3SS(chunk->blocks, off);
}

uint8_t *get_lum(ivec3 pos) {
    struct chunk *chunk;
    ivec3 off;

    if (!in_map(pos)) {
        return NULL;
    }
    chunk = get_chunk(pos);
    chunk_offset(pos, off);
    return &V3SS(chunk->lums, off);
}

int get_block_or(ivec3 v, int b) {
    uint8_t *p;

    p = get_block(v);
    return p ? *p : b;
}

void mark_chunk_dirty(struct chunk *chunk) {
    dirty_chunks[n_dirty_chunks++] = chunk;
    chunk->dirty = 1;
}
