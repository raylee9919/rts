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
