#pragma once

#include <stdint.h>
#include <cglm/ivec3.h>

#define CHUNK_LEN 16 
#define CHUNK_MASK (CHUNK_LEN - 1)
#define NX_CHUNK 8
#define NY_CHUNK 1
#define NZ_CHUNK 8
#define BLOCKS_IN_CHUNK (CHUNK_LEN * CHUNK_LEN * CHUNK_LEN)
#define N_CHUNKS (NX_CHUNK * NY_CHUNK * NZ_CHUNK)
#define BLOCKS_IN_WORLD (NX_CHUNK * NY_CHUNK * NZ_CHUNK * BLOCKS_IN_CHUNK) 

#define AIR   0
#define DIRT  1
#define GRASS 2
#define STONE 3

#define NEG_X 0
#define POS_X 1
#define NEG_Y 2
#define POS_Y 3
#define NEG_Z 4
#define POS_Z 5

#define MAX_LIGHTS (BLOCKS_IN_CHUNK * 2)

struct bounds {
    ivec3 min;
    ivec3 max;
};

struct light {
    ivec3 pos;
    int lum;
};

struct chunk {
    ivec3 pos;
    uint8_t blocks[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
    uint8_t faces[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
    uint8_t lums[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
    uint8_t heightmap[CHUNK_LEN][CHUNK_LEN];
    uint8_t outflow[6][CHUNK_LEN][CHUNK_LEN];
    int old_outflow;
    int new_outflow;
    int dirty;
    struct light lights[MAX_LIGHTS];
    struct light *head_light;
    struct light *tail_light;
};

extern struct chunk map[NX_CHUNK][NY_CHUNK][NZ_CHUNK];
extern struct chunk *dirty_chunks[N_CHUNKS];
extern int n_dirty_chunks;
extern ivec3 chunk_dim;

int in_chunk(ivec3 pos);
int in_map(ivec3 pos);
uint8_t *get_block(ivec3 pos);
uint8_t *get_lum(ivec3 pos);
int get_block_or(ivec3 v, int b);
void mark_chunk_dirty(struct chunk *chunk);
