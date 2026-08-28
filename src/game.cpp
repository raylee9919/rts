// Copyright Seong Woo Lee. All Rights Reserved.

void game_state_init(Game_State **game_state_pptr) {
    // It's virtual memory.
    u64 sz = 1ull * 1024 * 1024 * 1024;
    u8 *ptr = (u8*)os_reserve(sz);
    Assert( os_commit(ptr, sz) );

    Game_State *g = (Game_State *)ptr;
    Construct(g);

    g->storage.used     += sizeof(Game_State);
    g->storage.reserved  = sz;
    g->storage.base      = ptr;

    *game_state_pptr = g;
}

void game_init(f64 time_init) {
    game_state_init(&game_state);
    game_state->time = time_init;
    log(LOG_INFO, S("Initialized game state."));
}

void game_shutdown() {
    Game_State *g = game_state;

    os_release(g->storage.base, g->storage.reserved);

    log(LOG_INFO, S("Shutdown game states."));
}

void game_copy(Game_State *dst, Game_State *src) {
    ProfileScope

    u8 *tmp = dst->storage.base;
    memcpy(dst->storage.base, src->storage.base, src->storage.used);
    dst->storage.base = tmp;
}

Entity *entity_alloc(Game_State *g) {
    Entity *entity = NULL;

    // @Todo: Pool

    u64 offset = g->storage.used;
    g->entities[g->next_generational_id - 1] = offset; // @Temporary

    {
        Assert(g->storage.used + sizeof(Entity) <= g->storage.reserved);
        entity = (Entity *)(g->storage.base + g->storage.used);
        g->storage.used += sizeof(Entity);
    }

    entity->generational_id = g->next_generational_id++;

    return entity;
}

void entity_dealloc(Game_State *g) { 

}

Entity *entity_from_offset(Game_State *g, u64 offset) {
    Entity *entity = (Entity *)(g->storage.base + offset);
    return entity;
}
