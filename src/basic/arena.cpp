// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/arena.h"
#include "os/os.h"

Arena* arena_alloc_(u64 rsv_size_in, u64 cmt_size_in)
{
    u64 page_size = os_query_page_size();
    u64 rsv_size  = align_up(rsv_size_in, page_size);
    u64 cmt_size  = align_up(cmt_size_in, page_size);

    void *base = os_reserve(rsv_size);
    R_ASSERT(base != 0);
    os_commit(base, cmt_size);

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

    asan_poison(arena, cmt_size);
    asan_unpoison(arena, ARENA_HEADER_SIZE);

    return arena;
}

void arena_release(Arena *arena)
{
    for (Arena* n = arena->current, *prev = 0; n != 0; n = prev)
    {
        prev = n->prev;
        os_release(n, n->rsv);
    }
}

void* arena_push(Arena *arena, u64 size, u64 align)
{
    Arena *current = arena->current;
    u64 pos_pre = align_up(current->pos, align);
    u64 pos_pst = pos_pre + size;

    if (current->rsv < pos_pst)
    {
        u64 rsv_size = current->rsv_size;
        u64 cmt_size = current->cmt_size;
        if (size + ARENA_HEADER_SIZE > rsv_size)
        {
            rsv_size = align_up(size + ARENA_HEADER_SIZE, align);
            cmt_size = align_up(size + ARENA_HEADER_SIZE, align);
        }
        Arena *new_block = arena_alloc_(rsv_size, cmt_size);

        new_block->base_pos = current->base_pos + current->rsv;
        new_block->prev = arena->current;
        arena->current = new_block;

        current = new_block;
        pos_pre = align_up(current->pos, align);
        pos_pst = pos_pre + size;
    }

    if (current->cmt < pos_pst)
    {
        u64 cmt_pst_aligned = pos_pst + current->cmt_size-1;
        cmt_pst_aligned -= cmt_pst_aligned%current->cmt_size;
        u64 cmt_pst_clamped = min(cmt_pst_aligned, current->rsv);
        u64 cmt_size = cmt_pst_clamped - current->cmt;
        u8 *cmt_ptr = (u8 *)current + current->cmt;
        os_commit(cmt_ptr, cmt_size);
        current->cmt = cmt_pst_clamped;
    }

    void *result = 0;
    if (current->cmt >= pos_pst)
    {
        result = (u8 *)current+pos_pre;
        current->pos = pos_pst;
        asan_unpoison(result, size);
    }

    return result;
}

u64 arena_pos(Arena *arena)
{
    Arena *current = arena->current;
    u64 pos = current->base_pos + current->pos;
    return pos;
}

void arena_pop_to(Arena *arena, u64 pos)
{
    u64 big_pos = max(ARENA_HEADER_SIZE, pos);
    Arena *current = arena->current;

    for (Arena *prev = 0; current->base_pos >= big_pos; current = prev)
    {
        prev = current->prev;
        os_release(current, current->rsv);
    }

    arena->current = current;
    u64 new_pos = big_pos - current->base_pos;
    R_ASSERT(new_pos <= current->pos);
    asan_poison((u8 *)current + new_pos, (current->pos - new_pos));
    current->pos = new_pos;
}

void arena_clear(Arena *arena)
{
    arena_pop_to(arena, 0);
}

void arena_pop(Arena *arena, u64 size)
{
    u64 pos_old = arena_pos(arena);
    u64 pos_new = pos_old;
    if (size < pos_old)
    {
        pos_new = pos_old - size;
    }
    arena_pop_to(arena, pos_new);
}

Temporary_Arena temporary_arena_begin(Arena *arena)
{
    u64 pos = arena_pos(arena);
    Temporary_Arena temp = {};
    temp.arena = arena;
    temp.pos = pos;
    return temp;
}

void temporary_arena_end(Temporary_Arena temp)
{
    arena_pop_to(temp.arena, temp.pos);
}

void *arena_allocator_proc(Allocator_Mode mode, u64 size, u64 old_size, void *old_memory, void *data) {
    Arena *arena = (Arena *)data;

    if (mode == ALLOCATOR_MODE_ALLOCATE) {
        return push_size(arena, size);
    } else if (mode == ALLOCATOR_MODE_RESIZE) {
        void *ptr = push_size(arena, size);
        memcpy(ptr, old_memory, old_size);
        return ptr;
    } else if (mode == ALLOCATOR_MODE_FREE) {
        return NULL;
    } else if (mode == ALLOCATOR_MODE_RELEASE) {
        arena_release(arena);
        return NULL;
    } else {
        R_ASSERT(!"X");
        return NULL;
    }
}

Allocator arena_allocator_alloc() {
    Allocator result = {};
    result.data = arena_alloc();
    result.proc = arena_allocator_proc;
    return result;
}
