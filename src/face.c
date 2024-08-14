#include "face.h"
#include "map.h"
#include "dirty.h"
#include "misc.h"

#define LEFT     1U
#define RIGHT    2U
#define BOTTOM   4U
#define TOP      8U
#define BACK    16U
#define FRONT   32U

static void chunk_gen_faces(struct chunk *chunk) {
    for (int x = 0; x < CHUNK_LEN; x++) {
        for (int y = 0; y < CHUNK_LEN; y++) {
            for (int z = 0; z < CHUNK_LEN; z++) {
                if (!chunk->blocks[x][y][z]) {
                    continue;
                }
                chunk->faces[x][y][z] = RIGHT | FRONT | TOP;
                if (x == 0 || !chunk->blocks[x - 1][y][z]) {
                   chunk->faces[x][y][z] |= LEFT; 
                } else {
                   chunk->faces[x - 1][y][z] &= ~RIGHT; 
                }
                if (y == 0 || !chunk->blocks[x][y - 1][z]) {
                   chunk->faces[x][y][z] |= BOTTOM; 
                } else {
                   chunk->faces[x][y - 1][z] &= ~TOP; 
                }
                if (z == 0 || !chunk->blocks[x][y][z - 1]) {
                   chunk->faces[x][y][z] |= BACK; 
                } else {
                   chunk->faces[x][y][z - 1] &= ~FRONT; 
                }
            }
        }
    }
}

static void stub(struct chunk *) {};

void world_gen_faces(void) {
    do_dirty_jobs(DIRTY_FACES, chunk_gen_faces); 
    clear_dirty_all(DIRTY_FACES, stub); 
}

