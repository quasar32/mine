#include <assert.h>
#include "dirty.h"

struct dirty dirties[N_DIRITES];

void mark_dirty(struct chunk *chunk, int type) {
    int flag = 1U << type;
    if (!(chunk->dirty & flag)) {
        struct dirty *dirty = &dirties[type];
        assert(dirty->n_chunks < N_CHUNKS); 
        dirty->chunks[dirty->n_chunks++] = chunk;
        chunk->dirty |= (1U << type);
    }
}

void mark_dirty_all(struct chunk *chunk) {
    for (int type = 0; type < N_DIRITES; type++) {
        mark_dirty(chunk, type);
    }
}

void clear_dirty(struct chunk *chunk, int type) {
    chunk->dirty &= ~(1U << type);
}

void clear_dirty_all(int type, void(*clear)(struct chunk *)) {
    struct dirty *dirty = &dirties[type];
    for (int i = 0; i < dirty->n_chunks; i++) {
        struct chunk *chunk = dirty->chunks[i];
        clear(chunk);
        clear_dirty(chunk, type);
    }
    dirty->n_chunks = 0;
}
