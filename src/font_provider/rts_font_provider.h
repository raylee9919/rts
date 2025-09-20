/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Font_Atlas
{
    Arena *arena;

    u32 width;
    u32 height;
    u32 pitch;
    u8 *data;

    Rpk_Context *rpk_ctx;
};
