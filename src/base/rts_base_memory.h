#ifndef RTS_MEMORY_H
#define RTS_MEMORY_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Memory_Arena 
{
    umm     size;
    umm     used;
    void    *base;

    u32 temp_count;
};

struct Temporary_Memory 
{
    Memory_Arena *memory_arena;
    umm used;
};

struct Work_Memory_Arena 
{
    b32               is_used;
    Memory_Arena      arena;
    Temporary_Memory  flush;
};

#define push_struct(arena, type)            (type *)push_size(arena, sizeof(type))
#define push_array(arena, type, count)      (type *)push_size(arena, count * sizeof(type))
#define push_copy(arena, src, size)         copy(src, push_size(arena, size), size)
internal void *push_size(Memory_Arena *arena, size_t size);
internal void init_arena(Memory_Arena *arena, void *base, umm size);
internal void init_subarena(Memory_Arena *sub_arena, Memory_Arena *mom_arena, umm size);
internal Temporary_Memory begin_temporary_memory(Memory_Arena *memory_arena);
internal void end_temporary_memory(Temporary_Memory *temporary_memory);

#endif // RTS_MEMORY_H
