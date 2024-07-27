#include "object.h"
#include "map.h"
#include "misc.h"
#include "hotbar.h"

#include <math.h>
#include <GLFW/glfw3.h>

static double t0;

struct object player = {
    .type = OBJ_PLAYER,
    .pos = {.x = 8.0F, .y = 64.0F, .z = 8.0F}
};

struct prism locals[] = {
    [OBJ_PLAYER] = {
        {.x = -0.3F, .z = -0.3F},
        {.x = 0.3F, .y = 1.8F, .z = 0.3F} 
    },
    [OBJ_ITEM] = {
        {.x = -0.25F, .y = -0.25F, .z = -0.25F},
        {.x = 0.25F, .y = 0.25F, .z = 0.25F},
    }
};

float dt;

struct object items[BLOCKS_IN_WORLD];
int n_items;

static void get_detect_bounds(struct prism *prism, struct bounds *bounds) {
    for (int i = 0; i < 3; i++) {
        bounds->min.raw[i] = floorf(prism->min.raw[i] + 0.5F);
        bounds->max.raw[i] = ceilf(prism->max.raw[i] + 0.5F);
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
    if (obj->vel.raw[axis] < 0.0F) {
        disp->max.raw[axis] = disp->min.raw[axis];
        disp->min.raw[axis] += obj->vel.raw[axis] * dt;
    } else {
        disp->min.raw[axis] = disp->max.raw[axis];
        disp->max.raw[axis] += obj->vel.raw[axis] * dt;
    }
}

int prism_collide(struct prism *a, struct prism *b) {
    return a->max.x > b->min.x && b->max.x > a->min.x &&
           a->max.y > b->min.y && b->max.y > a->min.y &&
           a->max.z > b->min.z && b->max.z > a->min.z;
}

void get_block_prism(struct prism *prism, ivec3s pos) {
    prism->min.x = pos.x - 0.5F;
    prism->max.x = pos.x + 0.5F;
    prism->min.y = pos.y - 0.5F;
    prism->max.y = pos.y + 0.5F;
    prism->min.z = pos.z - 0.5F;
    prism->max.z = pos.z + 0.5F;
}

void get_world_prism(struct prism *world, struct object *obj) {
    struct prism *local;
    
    local = locals + obj->type;
    world->min = glms_vec3_add(obj->pos, local->min);
    world->max = glms_vec3_add(obj->pos, local->max);
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
    struct bounds bounds;
    ivec3s pos;
    float *blockf, *dispf, *posf, *velf;
    float (*sel)(float, float);
    float localf, off;
    int axis, cols;

    obj->flags &= ~GROUNDED;
    local = locals + obj->type;
    if (obj->flags & STUCK) {
        for (axis = 0; axis < 3; axis++) {
            velf = obj->vel.raw + axis;
            if (*velf == 0.0F) {
                continue;
            }
            get_disp_prism(obj, &disp, axis);
            get_detect_bounds(&disp, &bounds);
            posf = obj->pos.raw + axis;
            if (*velf < 0.0F) {
                dispf = disp.min.raw + axis;
                localf = local->min.raw[axis];
                off = 0.5F + localf * 2;
                blockf = block.max.raw + axis;
                sel = fmaxf;
            } else {
                dispf = disp.max.raw + axis;
                localf = local->max.raw[axis];
                off = -0.5F + localf * 2;
                blockf = block.min.raw + axis;
                sel = fminf;
            }
            cols = 0;
            FOR_BOUNDS (pos, bounds) {
                if (!get_block_or(pos, 0)) {
                    get_block_prism(&block, pos);
                    *blockf = pos.raw[axis] + off;
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
            velf = obj->vel.raw + axis;
            if (*velf == 0.0F) {
                continue;
            }
            get_disp_prism(obj, &disp, axis);
            get_detect_bounds(&disp, &bounds);
            posf = obj->pos.raw + axis;
            if (*velf < 0.0F) {
                dispf = disp.min.raw + axis;
                localf = local->min.raw[axis];
                off = 0.5F;
                sel = fmaxf;
            } else {
                dispf = disp.max.raw + axis;
                localf = local->max.raw[axis];
                off = -0.5F;
                sel = fminf;
            }
            cols = 0;
            FOR_BOUNDS (pos, bounds) {
                if (get_block_or(pos, 0)) {
                    *dispf = sel(pos.raw[axis] + off, *dispf);
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
        obj->vel.y = fmaxf(obj->vel.y - 24.0F * dt, -64.0F);
    }
}
