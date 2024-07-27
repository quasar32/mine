#include "map.h"
#include "misc.h"
#include "heightmap.h"

#define IMPL_GET(prop) \
    uint8_t *get_##prop(ivec3s world_pos) { \
        if (!world_pos_in_world(world_pos)) { \
            return NULL; \
        } \
        struct chunk *chunk = world_pos_to_chunk(world_pos); \
        ivec3s local_pos = world_to_local_pos(world_pos); \
        return get_local_##prop(chunk, local_pos); \
    }

ivec3s nv_chunks = {
    .x = NX_CHUNKS, 
    .y = NY_CHUNKS, 
    .z = NZ_CHUNKS
};

struct chunk chunks[NX_CHUNKS][NY_CHUNKS][NZ_CHUNKS];

IMPL_GET(block);
IMPL_GET(lum);

int get_block_or(ivec3s v, int b) {
    uint8_t *p;

    p = get_block(v);
    return p ? *p : b;
}
