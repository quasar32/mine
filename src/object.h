#ifndef OBJECT_H
#define OBJECT_H

#include <cglm/vec3.h>
#include <cglm/ivec3.h>

#define OBJ_PLAYER 0
#define OBJ_ITEM   1

#define GROUNDED 1U
#define STUCK    2U

struct prism {
    vec3 min;
    vec3 max;
};

struct object {
    int type;
    unsigned flags;
    vec3 pos;
    vec3 vel;
    int id;
    float rot;
};

extern struct prism locals[];
extern float dt;

extern struct object items[];
extern int n_items;

extern struct object player;

int prism_collide(struct prism *a, struct prism *b);
void get_block_prism(struct prism *prism, ivec3 pos);
void get_world_prism(struct prism *world, struct object *obj);
void update_dt(void);
void move(struct object *obj);

#endif
