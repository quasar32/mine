#include "light.h"
#include "map.h"
#include "misc.h"

#include <string.h>

static void next_light(struct chunk *chunk, struct light **light) {
    ++*light;    
    if (*light == chunk->lights + MAX_LIGHTS) {
        *light = chunk->lights;
    }
}

static void enqueue_light(struct chunk *chunk, struct light *light) {
    if (!chunk->head_light) {
        chunk->head_light = chunk->lights;
        chunk->tail_light = chunk->lights;
    } else if (chunk->head_light == chunk->tail_light) {
        die("too many lights\n");
    }
    *chunk->tail_light = *light;
    next_light(chunk, &chunk->tail_light);
}

static int dequeue_light(struct chunk *chunk, struct light *light) {
    if (!chunk->head_light) {
        return 0;
    }
    *light = *chunk->head_light;
    next_light(chunk, &chunk->head_light);
    if (chunk->head_light == chunk->tail_light) {
        chunk->head_light = NULL;
        chunk->tail_light = NULL;
    }
    return 1;
}

static int get_dir_outbounds(ivec3 pos) {
    int axis;

    for (axis = 0; axis < 3; axis++) {
        if (pos[axis] < 0) {
            return axis * 2;
        }
        if (pos[axis] >= CHUNK_LEN) {
            return axis * 2 + 1;
        }
    }
    return -1;
}

static void other_axes(int axis, ivec2 axes) {
    axes[0] = axis == 0 ? 1 : 0;
    axes[1] = axis == 2 ? 1 : 2;
}

static void enqueue_outflow_one(struct chunk *in, 
                                struct chunk *out,
                                int axis,
                                int dir) {
    int axes[2]; 
    int x, y;
    struct light light;
    uint8_t *lum;

    if ((out->old_outflow & (1 << dir))) {
        other_axes(axis, axes);
        for (x = 0; x < CHUNK_LEN; x++) {
            for (y = 0; y < CHUNK_LEN; y++) {
                light.pos[axis] = dir % 2 ? 0 : CHUNK_LEN - 1;
                light.pos[axes[0]] = x;
                light.pos[axes[1]] = y;
                light.lum = out->outflow[dir][x][y];
                lum = &V3SS(in->lums, light.pos); 
                if (*lum < light.lum) {
                    *lum = light.lum;
                    enqueue_light(in, &light);
                }
            }
        } 
    }
}

static void enqueue_outflow_all(struct chunk *in) {
    ivec3 pos;
    int axis;
    int dir;
    struct chunk *out;

    for (axis = 0; axis < 3; axis++) {
        if (in->pos[axis] > 0) {
            glm_ivec3_copy(in->pos, pos);
            pos[axis]--; 
            out = &V3SS(map, pos);
            dir = axis * 2 + 1; 
            enqueue_outflow_one(in, out, axis, dir);
        } 
        if (in->pos[axis] < chunk_dim[axis] - 1) {
            glm_ivec3_copy(in->pos, pos);
            pos[axis]++;
            out = &V3SS(map, pos);
            dir = axis * 2; 
            enqueue_outflow_one(in, out, axis, dir);
        }
    }
}

#include <stdio.h>
static void spread_light(struct chunk *chunk, struct light *old) {
    int face;
    struct light new;
    uint8_t *lum;
    uint8_t *block;
    int dir;
    int axis;
    ivec2 axes;
    ivec2 pos;

    for (face = 0; face < 6; face++) {
        new = *old;
        new.pos[face / 2] -= face_dir(face);
        new.lum--;
        dir = get_dir_outbounds(new.pos);
        if (dir < 0) {
            block = &V3SS(chunk->blocks, new.pos); 
            lum = &V3SS(chunk->lums, new.pos); 
            if (*lum < new.lum && !*block) {
                *lum = new.lum;
                if (new.lum != 0) {
                    enqueue_light(chunk, &new);
                }
            }
        } else {
            axis = dir / 2;
            other_axes(axis, axes);
            pos[0] = new.pos[axes[0]] & CHUNK_MASK;
            pos[1] = new.pos[axes[1]] & CHUNK_MASK;
            block = &chunk->outflow[dir][pos[0]][pos[1]];
            if (new.lum > *block) {
                *block = new.lum;
                chunk->new_outflow |= 1 << dir;
            }
        }
    }
}

void gen_lums(void) {
    int x, y, z;
    struct light light;
    struct chunk *chunk;
    int i;
    int outflow;

    for (i = 0; i < n_dirty_chunks; i++) {
        chunk = dirty_chunks[i];
        for (x = 0; x < CHUNK_LEN; x++) {
            for (z = 0; z < CHUNK_LEN; z++) {
                chunk->heightmap[x][z] = 0;
                for (y = 0; y < CHUNK_LEN; y++) {
                    if (chunk->blocks[x][y][z]) {
                        chunk->heightmap[x][z] = y;
                    }
                }
                light.pos[0] = x;
                light.pos[1] = chunk->heightmap[x][z] + 1;
                light.pos[2] = z;
                light.lum = 15;
                enqueue_light(chunk, &light);
            }
        }
        memset(chunk->lums, 0, sizeof(chunk->lums));
        memset(chunk->outflow, 0, sizeof(chunk->outflow));
        chunk->old_outflow = 0;
        chunk->new_outflow = 0;
    }
    do {
        outflow = 0;
        for (i = 0; i < n_dirty_chunks; i++) {
            chunk = dirty_chunks[i];
            chunk->old_outflow = chunk->new_outflow;
            chunk->new_outflow = 0;
        }
        for (i = 0; i < n_dirty_chunks; i++) {
            chunk = dirty_chunks[i];
            enqueue_outflow_all(chunk);
            while (dequeue_light(chunk, &light)) {
                spread_light(chunk, &light);
            }
            outflow |= chunk->new_outflow;
        }
    } while (outflow);
    for (i = 0; i < n_dirty_chunks; i++) {
        chunk = dirty_chunks[i];
        for (x = 0; x < CHUNK_LEN; x++) {
            for (z = 0; z < CHUNK_LEN; z++) {
                y = CHUNK_LEN;
                while (--y > chunk->heightmap[x][z]) {
                    chunk->lums[x][y][z] = 15;
                }
            }
        }
        chunk->new_outflow = 63;
    }
}
