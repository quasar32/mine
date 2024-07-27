#pragma once

#include "map.h"

struct heightmap {
    int chunk_x, chunk_z;
    int heights[CHUNK_LEN][CHUNK_LEN];
};

extern struct heightmap heightmaps[NX_CHUNKS][NZ_CHUNKS];

void world_gen_heightmap(void);

static inline struct heightmap *chunk_to_heightmap(struct chunk *chunk) {
    return &heightmaps[chunk->pos.x][chunk->pos.z];
}
