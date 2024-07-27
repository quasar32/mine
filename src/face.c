#include "face.h"
#include "map.h"
#include "dirty.h"

#define LEFT     1U
#define RIGHT    2U
#define BOTTOM   4U
#define TOP      8U
#define BACK    16U
#define FRONT   32U

static void chunk_gen_faces(struct chunk *chunk) {
    ivec3s old, new;
    ivec3s neg_chunk_pos;
    int axis;
    uint8_t *face, *neg_face;
    struct chunk *neg_chunk;
    int block;

    FOR_LOCAL_POS (old) {
        block = *get_local_block(chunk, old);
        if (!block) {
            continue;
        } 
        face = get_local_face(chunk, old);
        *face = FRONT | RIGHT | TOP; 
        for (axis = 0; axis < 3; axis++) {
            new = old;
            new.raw[axis]--;
            if (new.raw[axis] < 0) {
                neg_chunk_pos = chunk->pos;
                neg_chunk_pos.raw[axis]--;
                if (neg_chunk_pos.raw[axis] < 0) {
                    block = 0;
                } else { 
                    neg_chunk = chunk_pos_to_chunk(neg_chunk_pos);
                    new.raw[axis] += CHUNK_LEN;
                    block = *get_local_block(neg_chunk, new);
                    neg_face = get_local_face(neg_chunk, new);
                }
            } else {
                block = *get_local_block(chunk, new);
                neg_face = get_local_face(chunk, new); 
            } 
            if (block) {
                *neg_face &= ~(1 << (axis * 2 + 1)); 
            } else {
                *face |= 1 << (axis * 2);
            }
        }
    } 
}

void world_gen_faces(void) {
    clear_dirty_all(DIRTY_FACES, chunk_gen_faces); 
}

