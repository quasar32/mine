#include "object.h"
#include "map.h"
#include "misc.h"
#include "hotbar.h"

#include <math.h>
#include <GLFW/glfw3.h>

#define V3_FOR(pos, min, max) \
    for (pos[0] = min[0]; pos[0] < max[0]; pos[0]++) \
        for (pos[1] = min[1]; pos[1] < max[1]; pos[1]++) \
            for (pos[2] = min[2]; pos[2] < max[2]; pos[2]++) 

static double t0;

struct object player = {
    .type = OBJ_PLAYER,
    .pos = {8.0F, 16.0F, 8.0F}
};

struct prism locals[] = {
    [OBJ_PLAYER] = {
        {-0.3F, 0.0F, -0.3F},
        {0.3F, 1.8F, 0.3F} 
    },
    [OBJ_ITEM] = {
        {-0.25F, -0.25F, -0.25F},
        {0.25F, 0.25F, 0.25F},
    }
};

float dt;

struct object items[BLOCKS_IN_WORLD];
int n_items;

static void get_detect_bounds(struct prism *prism, ivec3 min, ivec3 max) {
    int i;

    for (i = 0; i < 3; i++) {
        min[i] = floorf(prism->min[i] + 0.5F);
        max[i] = ceilf(prism->max[i] + 0.5F);
    }
}

static void player_item_col(struct prism *disp) {
    struct prism block; 
    int i;

    for (i = 0; i < n_items; ) {
        get_world_prism(&block, items + i);
        if (prism_collide(disp, &block) &&
                add_to_hotbar(items[i].id)) {
            items[i] = items[--n_items];
        } else {
            i++;
        }
    }
}

static void get_disp_prism(struct object *obj, struct prism *disp, int axis) {
    get_world_prism(disp, obj);
    if (obj->vel[axis] < 0.0F) {
        disp->max[axis] = disp->min[axis];
        disp->min[axis] += obj->vel[axis] * dt;
    } else {
        disp->min[axis] = disp->max[axis];
        disp->max[axis] += obj->vel[axis] * dt;
    }
}

int prism_collide(struct prism *a, struct prism *b) {
    return a->max[0] > b->min[0] && b->max[0] > a->min[0] &&
           a->max[1] > b->min[1] && b->max[1] > a->min[1] &&
           a->max[2] > b->min[2] && b->max[2] > a->min[2];
}

void get_block_prism(struct prism *prism, ivec3 pos) {
    int i;

    for (i = 0; i < 3; i++) {
        prism->min[i] = pos[i] - 0.5F;
        prism->max[i] = pos[i] + 0.5F;
    }
}

void get_world_prism(struct prism *world, struct object *obj) {
    struct prism *local;
    
    local = locals + obj->type;
    glm_vec3_add(obj->pos, local->min, world->min);
    glm_vec3_add(obj->pos, local->max, world->max);
}

void update_dt(void) {
    float t1;

    t1 = glfwGetTime();
    dt = CLAMP(t1 - t0, 0.0F, 1.0F);
    t0 = t1;
}

void move(struct object *obj) {
    struct prism disp, block;
    struct prism *local;
    ivec3 pos, min, max;
    float *blockf, *dispf, *posf, *velf;
    float (*sel)(float, float);
    float localf, off;
    int axis, cols;

    obj->flags &= ~GROUNDED;
    local = locals + obj->type;
    if (obj->flags & STUCK) {
        for (axis = 0; axis < 3; axis++) {
            velf = obj->vel + axis;
            if (*velf == 0.0F) {
                continue;
            }
            get_disp_prism(obj, &disp, axis);
            get_detect_bounds(&disp, min, max);
            posf = obj->pos + axis;
            if (*velf < 0.0F) {
                dispf = disp.min + axis;
                localf = local->min[axis];
                off = 0.5F + localf * 2;
                blockf = block.max + axis;
                sel = fmaxf;
            } else {
                dispf = disp.max + axis;
                localf = local->max[axis];
                off = -0.5F + localf * 2;
                blockf = block.min + axis;
                sel = fminf;
            }
            cols = 0;
            V3_FOR (pos, min, max) {
                if (!get_block_or(pos, 0)) {
                    get_block_prism(&block, pos);
                    *blockf = pos[axis] + off;
                    if (prism_collide(&disp, &block)) {
                        *dispf = sel(*blockf, *dispf);
                        cols = 1;
                    }
                }
            }
            if (cols) {
                *velf = 0.0F;
                obj->flags &= ~STUCK;
            }
            *posf = *dispf - localf;
        }
    }
    if (!(obj->flags & STUCK)) {
        for (axis = 0; axis < 3; axis++) {
            velf = obj->vel + axis;
            if (*velf == 0.0F) {
                continue;
            }
            get_disp_prism(obj, &disp, axis);
            get_detect_bounds(&disp, min, max);
            posf = obj->pos + axis;
            if (*velf < 0.0F) {
                dispf = disp.min + axis;
                localf = local->min[axis];
                off = 0.5F;
                sel = fmaxf;
            } else {
                dispf = disp.max + axis;
                localf = local->max[axis];
                off = -0.5F;
                sel = fminf;
            }
            cols = 0;
            V3_FOR (pos, min, max) {
                if (get_block_or(pos, 0)) {
                    *dispf = sel(pos[axis] + off, *dispf);
                    cols = 1;
                }
            }
            if (cols) {
                if (*velf < 0.0F && axis == 1) {
                    obj->flags |= GROUNDED;
                }
                *velf = 0.0F;
            }
            if (obj->type == OBJ_PLAYER) {
                player_item_col(&disp);
            }
            *posf = *dispf - localf;
        }
        obj->vel[1] = fmaxf(obj->vel[1] - 24.0F * dt, -64.0F);
    }
}
