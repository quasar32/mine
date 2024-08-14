#pragma once

#include "render.h"
#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#define CGLM_OMIT_NS_FROM_STRUCT_API
#include <cglm/struct.h>

#define CHUNK_LEN 16 

#define NX_CHUNKS 16
#define NY_CHUNKS 16
#define NZ_CHUNKS 1

#define NX_BLOCKS (NX_CHUNKS * CHUNK_LEN)
#define NY_BLOCKS (NY_CHUNKS * CHUNK_LEN)
#define NZ_BLOCKS (NZ_CHUNKS * CHUNK_LEN)

#define BLOCKS_IN_CHUNK (CHUNK_LEN * CHUNK_LEN * CHUNK_LEN)
#define N_CHUNKS (NX_CHUNKS * NY_CHUNKS * NZ_CHUNKS)
#define BLOCKS_IN_WORLD (N_CHUNKS * BLOCKS_IN_CHUNK) 

#define AIR   0
#define DIRT  1
#define GRASS 2
#define STONE 3

#define MAX_LIGHTS (BLOCKS_IN_CHUNK * 4)

#define VERTICES_IN_CHUNK (36 * BLOCKS_IN_CHUNK) 

#define CHUNK_MASK 15

#define FOR_LOCAL_POS(pos) \
    for (pos.x = 0; pos.x < CHUNK_LEN; pos.x++) \
        for (pos.y = 0; pos.y < CHUNK_LEN; pos.y++) \
            for (pos.z = 0; pos.z < CHUNK_LEN; pos.z++) 

#define FOR_CHUNK_POS(pos) \
    for (pos.x = 0; pos.x < NX_CHUNKS; pos.x++) \
        for (pos.y = 0; pos.y < NY_CHUNKS; pos.y++) \
            for (pos.z = 0; pos.z < NZ_CHUNKS; pos.z++) 

#define FOR_CHUNK(pos, chunk) \
    FOR_CHUNK_POS (pos) \
        if (chunk = chunk_pos_to_chunk(pos), 1)

#define FOR_BOUNDS(pos, bounds) \
    for (pos.x = bounds.min.x; pos.x < bounds.max.x; pos.x++) \
        for (pos.y = bounds.min.y; pos.y < bounds.max.y; pos.y++) \
            for (pos.z = bounds.min.z; pos.z < bounds.max.z; pos.z++) 

#define IMPL_POS_IN(name, nx, ny, nz) \
    static inline int name(ivec3s pos) { \
        return pos.x >= 0 && pos.x < nx && \
               pos.y >= 0 && pos.y < ny && \
               pos.z >= 0 && pos.z < nz ; \
    } 

#define IMPL_LOCAL_GET(prop) \
    static inline uint8_t *get_local_##prop( \
            struct chunk *chunk, \
            ivec3s pos \
    ) { \
        assert(local_pos_in_chunk(pos)); \
        return &chunk->prop##s[pos.x][pos.y][pos.z]; \
    }

struct bounds {
    ivec3s min;
    ivec3s max;
};

struct light {
    ivec3s pos;
    int lum;
};

struct chunk {
    ivec3s pos;
    uint8_t blocks[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
    uint8_t faces[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
    uint8_t lums[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
    uint8_t heightmap[CHUNK_LEN][CHUNK_LEN];
    uint8_t outflow[6][CHUNK_LEN][CHUNK_LEN];
    int old_outflow;
    int new_outflow;
    unsigned dirty;
    struct light lights[MAX_LIGHTS];
    struct light *head_light;
    struct light *tail_light;
    uint32_t vertices[VERTICES_IN_CHUNK];
    int n_vertices;
    uint32_t vao;
    uint32_t vbo;
};

extern struct chunk chunks[NX_CHUNKS][NY_CHUNKS][NZ_CHUNKS];
extern ivec3s nv_chunks;

uint8_t *get_block(ivec3s pos);
uint8_t *get_lum(ivec3s pos);
int get_block_or(ivec3s v, int b);

IMPL_POS_IN(local_pos_in_chunk, CHUNK_LEN, CHUNK_LEN, CHUNK_LEN);
IMPL_POS_IN(chunk_pos_in_world, NX_CHUNKS, NY_CHUNKS, NZ_CHUNKS);
IMPL_POS_IN(world_pos_in_world, NX_BLOCKS, NY_BLOCKS, NZ_BLOCKS);

IMPL_LOCAL_GET(block);
IMPL_LOCAL_GET(face);
IMPL_LOCAL_GET(lum);

static inline struct chunk *chunk_pos_to_chunk(ivec3s pos) {
    assert(chunk_pos_in_world(pos));
    return &chunks[pos.x][pos.y][pos.z];
}

static inline struct chunk *chunk_pos_to_chunk_safe(ivec3s pos) {
    return chunk_pos_in_world(pos) ? chunk_pos_to_chunk(pos) : NULL;
}

static inline ivec3s world_to_local_pos(ivec3s pos) {
    assert(world_pos_in_world(pos));
    return (ivec3s) {
        .x = pos.x % CHUNK_LEN,
        .y = pos.y % CHUNK_LEN,
        .z = pos.z % CHUNK_LEN,
    };
}

static inline ivec3s world_to_chunk_pos(ivec3s pos) {
    assert(world_pos_in_world(pos));
    return ivec3_divs(pos, CHUNK_LEN);
}

static inline struct chunk *world_pos_to_chunk(ivec3s pos) {
    return chunk_pos_to_chunk(world_to_chunk_pos(pos));
}

static inline ivec3s chunk_to_world_pos(ivec3s pos) {
    return ivec3_scale(pos, CHUNK_LEN);
}

static inline ivec3s local_to_world_pos(ivec3s local_pos, ivec3s chunk_pos) {
    return ivec3_add(local_pos, chunk_to_world_pos(chunk_pos));
}

static inline uint8_t *get_world_block(ivec3s world_pos) {
    if (!world_pos_in_world(world_pos)) {
        return NULL;
    }
    struct chunk *chunk = world_pos_to_chunk(world_pos);
    ivec3s local_pos = world_to_local_pos(world_pos);
    return get_local_block(chunk, local_pos); 
}
