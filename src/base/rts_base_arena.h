#ifndef RTS_ARENA_H
#define RTS_ARENA_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define ARENA_DEFAULT_RESERVE_SIZE  KB(64)
#define ARENA_HEADER_ALIGNED_SIZE   40 

struct Arena 
{
    Arena *prev;
    Arena *current;

    u64 size;
    u64 used;
    u8 *base;
};
static_assert(ARENA_HEADER_ALIGNED_SIZE <= sizeof(Arena));

struct Arena_Position
{
    Arena *arena;
    u64 used;
};

struct Temporary_Arena 
{
    Arena *arena;
    Arena_Position position;
};


internal Arena *arena_alloc(u64 size);
internal void arena_release(Arena *arena);

internal void arena_pop_to(Arena *arena, Arena_Position position);
internal void arena_pop(Arena *arena, u64 size);
internal void arena_clear(Arena *arena);
internal void *arena_push(Arena *arena, u64 size, u64 align);

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
