/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


internal Arena *
arena_alloc(u64 size = ARENA_DEFAULT_RESERVE_SIZE)
{
    u64 arena_plus_requested_size = ARENA_HEADER_ALIGNED_SIZE + size;
    u64 page_size = os.query_page_size();
    u64 actual_size = align_pow2(arena_plus_requested_size, page_size);

    b32 commit_now = true;

    void *ptr = os.memory_reserve(actual_size, commit_now); // @Important: spec'ed to pass page-aligned size.
    Arena *result = (Arena *)ptr;
    {
        result->base     = (u8 *)ptr + ARENA_HEADER_ALIGNED_SIZE;
        result->size     = actual_size - ARENA_HEADER_ALIGNED_SIZE;
        result->used     = 0;
        result->prev     = 0;
        result->current  = result;
    }

    return result;
}

internal void
arena_release(Arena *arena)
{
    Arena *node = arena->current;
    while (node != NULL)
    {
        void *ptr = node;
        node = node->prev;
        os.memory_release(ptr);
    }
}

internal void
arena_pop_to(Arena *arena, Arena_Position position)
{
    for (Arena *n = arena->current; n != position.arena; n = n->prev)
    {
        n->used = 0;
    }

    position.arena->used = position.used;
    arena->current = position.arena;
}

internal void
arena_pop(Arena *arena, u64 size)
{
    Arena *node = arena->current;
    for (;;)
    {
        if (node == NULL)
        { break; }

        if (node->used >= size)
        {
            node->used -= size;
            arena->current = node;
            break;
        }
        else
        {
            size -= node->used;
            node->used = 0;
            arena->current = node->prev;
            node = node->prev;
        }
    }
}

internal void
arena_clear(Arena *arena)
{
    for (Arena *node = arena->current; node != NULL; node = node->prev)
    {
        node->used = 0; 
        arena->current = node;
    }
}

internal void *
arena_push(Arena *arena, u64 size, u64 align)
{
    void *result = 0;
    Arena *current = arena->current;
    u64 used_pre = align_pow2(current->used, align);
    u64 used_pst = used_pre + size;

    if (current->size < used_pst)
    {
        Arena *new_arena = arena_alloc(size);
        new_arena->prev = current;

        arena->current = new_arena;

        result = new_arena->base;
        new_arena->used += size;
    }
    else
    {
        result = current->base + used_pre;
        current->used = used_pst;
    }

    return result;
}

internal Temporary_Arena
scratch_begin(void)
{
    Temporary_Arena result = {};
    {
        result.arena = tctx.scratch_arena;

        result.position.arena = tctx.scratch_arena->current;
        result.position.used  = tctx.scratch_arena->current->used;
    }
    return result;
}

internal void
scratch_end(Temporary_Arena tmp)
{
    arena_pop_to(tmp.arena, tmp.position);
}
