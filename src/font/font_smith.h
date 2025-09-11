/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */
struct Memory_Arena {
    umm size;
    void *base;
    umm used;
};

Memory_Arena g_main_arena;

#define push_array(STRUCT, COUNT) (STRUCT *)push_size(sizeof(STRUCT) * COUNT)
#define push_struct(STRUCT) (STRUCT *)push_size(sizeof(STRUCT))

static void *
push_size(umm size) 
{
    void *result = 0;
    if (size + g_main_arena.used > g_main_arena.size) {
        fprintf(stderr, "[ERROR]: Not enough memory!\n");
        exit(1);
    }
    result = (u8 *)g_main_arena.base + g_main_arena.used;
    g_main_arena.used += size;

    return result;
}
