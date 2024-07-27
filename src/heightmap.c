#include "heightmap.h"
#include "misc.h"
#include "dirty.h"
#include "map.h"

struct heightmap heightmaps[NX_CHUNKS][NZ_CHUNKS];

static void chunk_gen_heightmap(struct chunk *chunk) {
    for (int x = 0; x < CHUNK_LEN; x++) {
        for (int z = 0; z < CHUNK_LEN; z++) {
            int height = 0;
            for (int y = 0; y < CHUNK_LEN; y++) {
                if (chunk->blocks[x][y][z]) {
                    height = y + 1;
                }
            }
            chunk->heightmap[x][z] = height;
        }
    }
}

static void column_gen_heightmap(ivec2s pos) {
    struct heightmap *hm = &heightmaps[pos.x][pos.y];
    for (int x = 0; x < CHUNK_LEN; x++) {
        for (int z = 0; z < CHUNK_LEN; z++) {
            int h = 0;
            for (int y = 0; y < NY_CHUNKS; y++) {
                struct chunk *chunk = &chunks[pos.x][y][pos.y];
                int chunk_h = chunk->heightmap[x][z];
                if (chunk_h) {
                    h = y * CHUNK_LEN + chunk_h;
                }
            }
            hm->heights[x][z] = h;
        }
    }
}

void world_gen_heightmap(void) {
    struct dirty *dirty = &dirties[DIRTY_HEIGHTMAP];
    for (int i = 0; i < dirty->n_chunks; i++) {
        struct chunk *chunk = dirty->chunks[i];
        chunk_gen_heightmap(chunk);
        clear_dirty(chunk, DIRTY_HEIGHTMAP);
    }
    uint8_t grid[NX_CHUNKS][NZ_CHUNKS] = {};
    for (int i = 0; i < dirty->n_chunks; i++) {
        ivec3s pos = dirty->chunks[i]->pos;
        uint8_t *slot = &grid[pos.x][pos.z];
        if (!*slot) {
            *slot = 1;
            column_gen_heightmap((ivec2s) {.x = pos.x, .y = pos.z});
        }
    }
    dirty->n_chunks = 0;
}
