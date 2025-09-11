/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Memory_Arena {
    umm size;
    umm used;
    u8 *base;

    u32 temp_count;
};

struct Temporary_Memory {
    Memory_Arena *memory_arena;
    umm used;
};

struct Work_Memory_Arena {
    b32               is_used;
    Memory_Arena      arena;
    Temporary_Memory  flush;
};

#define push_size(arena, size)              push_size_(arena, size)
#define push_struct(arena, type)            (type *)push_size_(arena, sizeof(type))
#define push_array(arena, type, count)      (type *)push_size_(arena, count * sizeof(type))
#define push_copy(arena, src, size)         copy(src, push_size(arena, size), size)
internal void *
push_size_(Memory_Arena *arena, size_t size) {
    Assert((arena->used + size) <= arena->size);
    void *result = (u8 *)arena->base + arena->used;
    arena->used += size;

    return result;
}

internal void
init_arena(Memory_Arena *arena, void *base, umm size) {
    arena->size = size;
    arena->base = (u8 *)base;
    arena->used = 0;
}

internal void
init_subarena(Memory_Arena *sub_arena, Memory_Arena *mom_arena, umm size) {
    Assert(mom_arena->size >= mom_arena->used + size);
    init_arena(sub_arena, (u8 *)mom_arena->base + mom_arena->used, size);
    mom_arena->used += size;
}

internal Temporary_Memory
begin_temporary_memory(Memory_Arena *memory_arena) {
    memory_arena->temp_count++;

    Temporary_Memory result = {};
    result.memory_arena = memory_arena;
    result.used = memory_arena->used;

    return result;
}

internal void
end_temporary_memory(Temporary_Memory *temporary_memory) {
    Memory_Arena *arena = temporary_memory->memory_arena;
    Assert(arena->used >= temporary_memory->used);
    zerosize(arena->base + temporary_memory->used, arena->used - temporary_memory->used);
    arena->used = temporary_memory->used;
    Assert(arena->temp_count > 0);
    arena->temp_count--;
}
