#include "light.h"
#include "map.h"
#include "misc.h"
#include "dirty.h"

#include <string.h>
#include <stdatomic.h>

static atomic_uint world_outflow;

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

static ivec2s other_axes(int axis) {
    return (ivec2s) {
        .x = axis == 0 ? 1 : 0,
        .y = axis == 2 ? 1 : 2
    };
}

static void enqueue_outflow_one(struct chunk *in, 
                                struct chunk *out,
                                int axis,
                                int dir) {
    ivec2s axes; 
    int x, y;
    struct light light;
    uint8_t *lum;

    if ((out->old_outflow & (1 << dir))) {
        axes = other_axes(axis);
        for (x = 0; x < CHUNK_LEN; x++) {
            for (y = 0; y < CHUNK_LEN; y++) {
                light.pos.raw[axis] = dir % 2 ? 0 : CHUNK_LEN - 1;
                light.pos.raw[axes.x] = x;
                light.pos.raw[axes.y] = y;
                if (!*get_local_block(in, light.pos)) {
                    light.lum = out->outflow[dir][x][y];
                    lum = get_local_lum(in, light.pos);
                    if (*lum < light.lum) {
                        *lum = light.lum;
                        enqueue_light(in, &light);
                    }
                }
            }
        }
    }
}

static void enqueue_outflow_all(struct chunk *in) {
    ivec3s pos;
    int axis;
    int dir;
    struct chunk *out;

    for (axis = 0; axis < 3; axis++) {
        if (in->pos.raw[axis] > 0) {
            pos = in->pos;
            pos.raw[axis]--; 
            out = chunk_pos_to_chunk(pos);
            dir = axis * 2 + 1; 
            enqueue_outflow_one(in, out, axis, dir);
        } 
        if (in->pos.raw[axis] < nv_chunks.raw[axis] - 1) {
            pos = in->pos;
            pos.raw[axis]++;
            out = chunk_pos_to_chunk(pos);
            dir = axis * 2; 
            enqueue_outflow_one(in, out, axis, dir);
        }
    }
}

static void spread_light(struct chunk *chunk, struct light *old) {
    struct light new;
    uint8_t *lum;
    uint8_t *block;
    int dir;
    int axis;
    ivec2s axes;
    ivec2s pos;

    for (dir = 0; dir < 6; dir++) {
        new = *old;
        axis = dir / 2;
        new.pos.raw[axis] += face_dir(dir);
        if (dir != 2 || new.lum != 15) {
            new.lum--;
        }
        if (local_pos_in_chunk(new.pos)) {
            block = get_local_block(chunk, new.pos); 
            lum = get_local_lum(chunk, new.pos); 
            if (*lum < new.lum && !*block) {
                *lum = new.lum;
                if (new.lum != 0) {
                    enqueue_light(chunk, &new);
                }
            }
        } else {
            axes = other_axes(axis);
            pos.x = new.pos.raw[axes.x];
            pos.y = new.pos.raw[axes.y];
            block = &chunk->outflow[dir][pos.x][pos.y];
            if (new.lum > *block) {
                *block = new.lum;
                chunk->new_outflow |= 1 << dir;
            }
        }
    }
}

static void enqueue_sunlight(struct chunk *chunk) {
    if (chunk->pos.y < NY_CHUNKS - 1) {
        return;
    }
    for (int x = 0; x < CHUNK_LEN; x++) {
        for (int z = 0; z < CHUNK_LEN; z++) {
            struct light l = {
                .pos = {
                    .x = x,
                    .y = CHUNK_LEN,
                    .z = z,
                },
                .lum = 15,
            };
            enqueue_light(chunk, &l);
        }
    }
}

static void set_all_outflow(struct chunk *chunk) {
    chunk->old_outflow = 63;
}

static void clear_outflow(struct chunk *chunk) {
    memset(chunk->lums, 0, sizeof(chunk->lums));
    memset(chunk->outflow, 0, sizeof(chunk->outflow));
    chunk->old_outflow = 0;
    chunk->new_outflow = 0;
}

static void setup_lights(struct chunk *chunk) {
    enqueue_sunlight(chunk);
    clear_outflow(chunk);
}

static void flood_light(struct chunk *chunk) {
    struct light light;
    while (dequeue_light(chunk, &light)) {
        spread_light(chunk, &light);
    }
    chunk->old_outflow = chunk->new_outflow; 
    chunk->new_outflow = 0;
    world_outflow |= chunk->old_outflow;
}

void world_gen_lums(void) {
    do_dirty_jobs(DIRTY_LUMS, setup_lights);
    do {
        do_dirty_jobs(DIRTY_LUMS, enqueue_outflow_all);
        world_outflow = 0;
        do_dirty_jobs(DIRTY_LUMS, flood_light);
    } while (world_outflow);
    clear_dirty_all(DIRTY_LUMS, set_all_outflow);
}
