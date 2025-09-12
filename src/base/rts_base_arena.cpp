/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


internal mmm
get_page_aligned_size(mmm size)
{
    mmm page_size = os.query_page_size();
    mmm result= align_pow2(size, page_size);
    return result;
}

internal Arena *
arena_alloc(mmm size = ARENA_DEFAULT_RESERVE_SIZE)
{
    mmm arena_plus_requested_size = ARENA_HEADER_ALIGNED_SIZE + size;
    mmm actual_size = get_page_aligned_size(arena_plus_requested_size);

    b32 commit_now = true;

    void *ptr = os.memory_reserve(actual_size, commit_now); // IMPORTANT: spec'ed to pass page-aligned size.
    Arena *result = (Arena *)ptr;
    {
        result->base     = (u8 *)ptr + ARENA_HEADER_ALIGNED_SIZE;
        result->size     = actual_size - ARENA_HEADER_ALIGNED_SIZE;
        result->used     = 0;
        result->next     = NULL;
        result->prev     = NULL;
        result->current  = result;
    }

    return result;
}

internal void
arena_dealloc(Arena *arena)
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
arena_pop(Arena *arena, mmm size)
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
            node->used = 0;
            arena->current = node->prev;
            size -= node->used;
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
arena_push_size(Arena *arena, mmm size, Arena_Flags flags)
{
    void *result = NULL;

    Arena *current = arena->current;

    if (current->used + size <= current->size)
    {
        result = current->base + current->used;
        current->used += size;
    }
    else
    {
        Arena *new_arena = arena_alloc(size);
        new_arena->prev = current;
        arena->current = new_arena;

        result = new_arena->base;
        new_arena->used += size;
    }

    if (! (flags & ARENA_PUSH_NO_ZERO))
    {
        zero_size(result, size);
    }

    return result;
}

internal Temporary_Arena
tmp_begin(Arena *arena)
{
    Temporary_Arena result = {};
    {
        result.arena  = arena;
        result.used   = arena->used;
    }
    return result;
}

internal void
tmp_end(Temporary_Arena tmp)
{
    arena_pop(tmp.arena, tmp.arena->used - tmp.used);
}

internal Temporary_Arena
scratch_begin(void)
{
    Temporary_Arena result = {};
    {
        result.arena  = tctx.scratch_arena;
        result.used   = tctx.scratch_arena->used;
    }
    return result;
}

internal void
scratch_end(Temporary_Arena tmp)
{
    arena_pop(tmp.arena, tmp.arena->used - tmp.used);
}
