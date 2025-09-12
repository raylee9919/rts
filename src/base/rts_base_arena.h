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
// #define ARENA_DEFAULT_COMMIT_SIZE   KB(64)
#define ARENA_HEADER_ALIGNED_SIZE 48

struct Arena 
{
    Arena *next;
    Arena *prev;
    Arena *current;

    mmm size;
    mmm used;
    u8 *base;
};

typedef u16 Arena_Flags;
enum
{
    ARENA_PUSH_NO_ZERO,
};

struct Temporary_Arena 
{
    Arena *arena;
    mmm used;
};


internal mmm get_page_aligned_size(mmm size);

internal Arena *arena_alloc(mmm size);
internal void arena_dealloc(Arena *arena);
internal void arena_pop(Arena *arena, mmm size);
internal void arena_clear(Arena *arena);
internal void *arena_push_size(Arena *arena, mmm size, Arena_Flags flags);

#define push_size(arena, size) arena_push_size(arena, size, 0)
#define push_size_noz(arena, size) arena_push_size(arena, size, ARENA_PUSH_NO_ZERO) // @Note: Use it only when you know what you are doing!
#define push_struct(arena, type) ((type *)push_size(arena, sizeof(type)))
#define push_array(arena, type, count) ((type *)push_size(arena, sizeof(type)*count))
#define push_array_noz(arena, type, count) ((type *)push_size_noz(arena, sizeof(type)*count))

internal Temporary_Arena tmp_begin(Arena *arena);
internal void tmp_end(Temporary_Arena tmp);

internal Temporary_Arena scratch_begin(void);
internal void scratch_end(Temporary_Arena tmp);


#endif // RTS_ARENA_H
