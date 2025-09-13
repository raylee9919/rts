#ifndef RTS_ARENA_H
#define RTS_ARENA_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define ARENA_DEFAULT_RESERVE_SIZE  MB(64)
#define ARENA_DEFAULT_COMMIT_SIZE   KB(64)
#define ARENA_HEADER_SIZE           128

struct Arena 
{
    Arena *prev;
    Arena *current;

    u64 cmt_size;
    u64 rsv_size;
    u64 base_pos;
    u64 pos;
    u64 cmt;
    u64 rsv;
};
static_assert(ARENA_HEADER_SIZE >= sizeof(Arena));

struct Temporary_Arena 
{
    Arena *arena;
    u64 pos;
};


internal Arena *arena_alloc_(u64 rsv_size, u64 cmt_size);
#define arena_alloc() arena_alloc_(ARENA_DEFAULT_RESERVE_SIZE, ARENA_DEFAULT_COMMIT_SIZE)
internal void arena_release(Arena *arena);

internal void *arena_push(Arena *arena, u64 size, u64 align);

internal u64 arena_pos(Arena *arena);
internal void arena_pop_to(Arena *arena, u64 pos);
internal void arena_clear(Arena *arena);
internal void arena_pop(Arena *arena, u64 size);
internal Temporary_Arena temporary_arena_begin(Arena *arena);
internal void temporary_arena_end(Temporary_Arena temp);

#define push_array_noz_aligned(a, T, n, align)  (T *)arena_push((a), sizeof(T)*(n), (align))
#define push_array_aligned(a, T, n, align)      (T *)zero_memory(push_array_noz_aligned(a, T, n, align), sizeof(T)*(n))
#define push_array_noz(a, T, n)                      push_array_noz_aligned(a, T, n, max(8, align_of(T)))
#define push_array(a, T, n)                          push_array_aligned(a, T, n, max(8, align_of(T)))

#define push_struct_noz(a, T)                        push_array_noz(a, T, 1)
#define push_struct(a, T)                            push_array(a, T, 1)

#define push_size(a, s)                              arena_push(a, s, 8)

internal Temporary_Arena scratch_begin(void);
internal void scratch_end(Temporary_Arena tmp);


#endif // RTS_ARENA_H
