// Copyright Seong Woo Lee. All Rights Reserved.

void game_state_init(Game_State **game_state_pptr) {
    // Reserve gigantic memory space and allocate game state in there.
    u64 page_size = os_query_page_size();
    u64 rsv = align_up(Gigabytes(128), page_size);
    u8 *ptr = (u8*)os_reserve(rsv);

    // First commit
    u64 sz = sizeof(Game_State);
    u64 cmt = align_up(sz, page_size);
    Assert( os_commit(ptr, cmt) );

    Game_State *g = (Game_State *)ptr;
    Construct(g);

    g->storage.base      = ptr;
    g->storage.reserved  = rsv;
    g->storage.committed = cmt;
    g->storage.used     += sz;
    g->storage.page_size = page_size;

    *game_state_pptr = g;
}

void game_state_deinit(Game_State *g) {
    os_release(g->storage.base, g->storage.reserved);
}

void game_init(f64 time_init) {
    // Initialize game state
    game_state_init(&game_state);
    game_state->time = time_init;

    // Allocate root entity
    Entity *root = entity_alloc(game_state);
    game_state->root = handle_from_entity(game_state, root);

    log(LOG_INFO, S("Initialized game state."));
}

void game_deinit() {
    Game_State *g = game_state;

    os_release(g->storage.base, g->storage.reserved);

    log(LOG_INFO, S("Shutdown game states."));
}

void game_copy(Game_State *dst, Game_State *src) {
    ProfileScope;

    if (dst->storage.used < src->storage.used) {
        game_alloc(dst, src->storage.used - dst->storage.used, 1);
    }

    Game_Storage tmp = dst->storage;

    memcpy(dst->storage.base, src->storage.base, src->storage.used);
    dst->storage = tmp;
    dst->storage.used = src->storage.used;
}

void *game_alloc(Game_State *g, u64 size, u64 alignment) {
    Game_Storage *s = &g->storage;

    u8 *ptr = (u8*)align_up((void*)(s->base + s->used), alignment);
    u8 *opl = ptr + size;
    s->used = opl - s->base;

    if (s->used > s->committed) {
        u64 commit_size = align_up(s->used - s->committed, s->page_size);
        Assert( os_commit(s->base + s->committed, commit_size) );
        s->committed += commit_size;
    }

    return ptr;
}

Entity *entity_alloc(Game_State *g) {
    Entity *entity = entity_from_handle(g, g->first_free_entity);

    if (entity) {
        if (g->first_free_entity == g->last_free_entity) {
            g->first_free_entity = {};
            g->last_free_entity  = {};
        } else {
            g->first_free_entity = entity->next;
        }
    } else {
        entity = (Entity *)game_alloc(g, sizeof(Entity), align_of(Entity));
    }

    Assert(entity);

    memset(entity, 0, sizeof(Entity));

    // Assign generational ID
    entity->generational_id = g->next_generational_id++;

    // Attach to root
    entity_add_child(g, g->root, handle_from_entity(g, entity));

    g->num_entities += 1;

    return entity;
}

void entity_dealloc(Game_State *g) { 
    // @Todo: Must remove from the tree, decommit page?
    g->num_entities -= 1;
}

Entity *entity_from_handle(Game_State *g, Handle handle) {
    if (handle.is_null()) return NULL;
    Entity *entity = (Entity *)(g->storage.base + handle.offset);
    if (entity->generational_id == handle.generational_id) {
        return entity;
    }
    return NULL;
}

Handle handle_from_entity(Game_State *g, Entity *entity) {
    Handle h;
    h.offset = (u8*)entity - g->storage.base;
    h.generational_id = entity->generational_id;
    return h;
}

void entity_add_child(Game_State *g, Handle parent, Handle child) {
    Entity *pa = entity_from_handle(g, parent);
    Entity *ch = entity_from_handle(g, child);

    if (!pa || !ch)  return;

    ch->parent = parent;

    if (pa->first.is_null() && pa->last.is_null()) {
        pa->first = child;
        pa->last  = child;
        ch->prev  = child;
        ch->next  = child;
    } else {
        Entity *first = entity_from_handle(g, pa->first);
        Entity *last  = entity_from_handle(g, pa->last);

        Assert(first && last); // Deallocated entity must have been removed

        first->prev = child;
        last->next  = child;
        ch->prev    = pa->last;
        ch->next    = pa->first;
        pa->last    = child;
    }
}

void entity_remove_child(Game_State *g, Handle parent, Handle child) {
    // @Todo
}

static u64 _entity_dfs_internal(Game_State *g, Handle root, u64 index, 
                                void (*proc)(Game_State *g, Entity *entity, u64 index))
{
    Entity *entity = entity_from_handle(g, root);
    if (!entity)  return index;

    if (root != g->root) {
        proc(g, entity, index);
        index += 1;
    }

    Handle first = entity->first;
    if (first.is_null())  return index;

    Handle child = first;

    for (;;) {
        index = _entity_dfs_internal(g, child, index, proc);

        Entity *child_entity = entity_from_handle(g, child);

        Assert(child_entity);  // Deallocated entity must have been removed

        child = child_entity->next;
        if (child == first)  break;
    }

    return index;
}

void entity_dfs(Game_State *g, Handle root, 
                void (*proc)(Game_State *g, Entity *entity, u64 index)) 
{
    _entity_dfs_internal(g, root, 0, proc);
}
