#pragma once

#include "map.h"

enum dirty_type {
    DIRTY_HEIGHTMAP,
    DIRTY_LUMS,
    DIRTY_FACES,
    DIRTY_VERTICES,
    N_DIRITES
};

struct dirty {
    int n_chunks;
    struct chunk *chunks[N_CHUNKS];
};

extern struct dirty dirties[N_DIRITES];

void mark_dirty(struct chunk *chunk, int type);
void mark_dirty_all(struct chunk *chunk);
void clear_dirty(struct chunk *chunk, int type);
void clear_dirty_all(int type, void(*clear)(struct chunk *));
