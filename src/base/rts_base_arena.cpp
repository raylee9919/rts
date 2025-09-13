/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



internal Arena *
arena_alloc_(u64 rsv_size_in, u64 cmt_size_in)
{
    u64 page_size = os.query_page_size();
    u64 rsv_size = align_pow2(rsv_size_in, page_size);
    u64 cmt_size = align_pow2(cmt_size_in, page_size);

    void *base = os.memory_reserve(rsv_size);
    os.memory_commit(base, cmt_size);

    Assert(base != 0);

    Arena *arena = (Arena *)base;
    {
        arena->current       = arena;
        arena->cmt_size      = cmt_size_in;
        arena->rsv_size      = rsv_size_in;
        arena->base_pos      = 0;
        arena->pos           = ARENA_HEADER_SIZE;
        arena->cmt           = cmt_size;
        arena->rsv           = rsv_size;
    }

    return arena;
}

internal void
arena_release(Arena *arena)
{
    for (Arena *n = arena->current, *prev = 0; n != 0; n = prev)
    {
        prev = n->prev;
        os.memory_release(n, n->rsv);
    }
}

internal void *
arena_push(Arena *arena, u64 size, u64 align)
{
    Arena *current = arena->current;
    u64 pos_pre = align_pow2(current->pos, align);
    u64 pos_pst = pos_pre + size;

    if (current->rsv < pos_pst)
    {
        u64 rsv_size = current->rsv_size;
        u64 cmt_size = current->cmt_size;
        if (size + ARENA_HEADER_SIZE > rsv_size)
        {
            rsv_size = align_pow2(size + ARENA_HEADER_SIZE, align);
            cmt_size = align_pow2(size + ARENA_HEADER_SIZE, align);
        }
        Arena *new_block = arena_alloc();

        new_block->base_pos = current->base_pos + current->rsv;
        arena->current = new_block;
        new_block->prev = arena->current;

        current = new_block;
        pos_pre = align_pow2(current->pos, align);
        pos_pst = pos_pre + size;
    }

    if (current->cmt < pos_pst)
    {
        u64 cmt_pst_aligned = pos_pst + current->cmt_size-1;
        cmt_pst_aligned -= cmt_pst_aligned%current->cmt_size;
        u64 cmt_pst_clamped = clamp_hi(cmt_pst_aligned, current->rsv);
        u64 cmt_size = cmt_pst_clamped - current->cmt;
        u8 *cmt_ptr = (u8 *)current + current->cmt;
        os.memory_commit(cmt_ptr, cmt_size);
        current->cmt = cmt_pst_clamped;
    }

    void *result = 0;
    if (current->cmt >= pos_pst)
    {
        result = (u8 *)current+pos_pre;
        current->pos = pos_pst;
    }

    return result;
}

internal u64
arena_pos(Arena *arena)
{
    Arena *current = arena->current;
    u64 pos = current->base_pos + current->pos;
    return pos;
}

internal void
arena_pop_to(Arena *arena, u64 pos)
{
    u64 big_pos = clamp_lo(ARENA_HEADER_SIZE, pos);
    Arena *current = arena->current;

    for (Arena *prev = 0; current->base_pos >= big_pos; current = prev)
    {
        prev = current->prev;
        os.memory_release(current, current->rsv);
    }

    arena->current = current;
    u64 new_pos = big_pos - current->base_pos;
    Assert(new_pos <= current->pos);
    current->pos = new_pos;
}

internal void
arena_clear(Arena *arena)
{
    arena_pop_to(arena, 0);
}

internal void
arena_pop(Arena *arena, u64 size)
{
    u64 pos_old = arena_pos(arena);
    u64 pos_new = pos_old;
    if (size < pos_old)
    {
        pos_new = pos_old - size;
    }
    arena_pop_to(arena, pos_new);
}

internal Temporary_Arena
temporary_arena_begin(Arena *arena)
{
    u64 pos = arena_pos(arena);
    Temporary_Arena temp = {};
    temp.arena = arena;
    temp.pos = pos;
    return temp;
}

internal void
temporary_arena_end(Temporary_Arena temp)
{
    arena_pop_to(temp.arena, temp.pos);
}

