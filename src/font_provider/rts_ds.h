/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


struct Array_Header
{
    u64 len;
    u64 cap;
};

#define arrsetlen(arena, ptr, count) _arrsetlen((arena), (void **)(&(ptr)), sizeof(*(ptr)), (count))
#define arrlenu(ptr) _arrlenu((void **)(&(ptr)))

internal void _arrsetlen(Arena *arena, void **arr, u64 size, u64 count);
internal u64 _arrlenu(void **arr);
