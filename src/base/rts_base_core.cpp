/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

internal u16
to_u16_safe(u32 x)
{
    Assert(x <= U16_MAX);
    u16 result = (u16)x;
    return result;
}

internal u32
to_u32_safe(u64 x)
{
    Assert(x <= U32_MAX);
    u32 result = (u32)x;
    return result;
}

internal s32
to_s32_safe(s64 x)
{
    Assert(x <= S32_MAX);
    s32 result = (s32)x;
    return result;
}

internal umm
to_raw(f32 val) 
{
    umm foo = *(umm *)&val;
    umm result = (foo & 0xffffffff);
    return result;
}

internal void *
_dll_np(void *node, u64 np)
{
    void *result = ptr_from_int(*(u64 *)((u8 *)node + np));
    return result;
}

internal void
_dll_sort(void *sentinel, u64 size, u64 next, u64 prev, int(*cmp)(void*,void*))
{
    Temporary_Arena scratch = scratch_begin();

    for (void *end = _dll_np(sentinel, prev);
         end != _dll_np(sentinel, next);
         end = _dll_np(end, prev))
    {
        for (void *it = _dll_np(sentinel, next);
             it != end;
             it = _dll_np(it, next))
        {
            void *in = _dll_np(it, next);
            if (cmp(it, in))
            {
                u8 *tmp1 = (u8 *)push_size(scratch.arena, size);
                u8 *tmp2 = (u8 *)push_size(scratch.arena, size);
                memory_copy(tmp1, it, size);
                memory_copy(tmp2, in, size);
                memory_copy(it, tmp2, size);
                memory_copy(in, tmp1, size);
                u8 *it8 = (u8 *)it;
                u8 *in8 = (u8 *)in;
                memory_copy(it8 + next, tmp1 + next, sizeof(void *));
                memory_copy(in8 + next, tmp2 + next, sizeof(void *));
                memory_copy(it8 + prev, tmp1 + prev, sizeof(void *));
                memory_copy(in8 + prev, tmp2 + prev, sizeof(void *));
            }
        }
    }

    scratch_end(scratch);
}
