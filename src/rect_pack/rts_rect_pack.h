#ifndef RTS_RECT_PACK_H
#define RTS_RECT_PACK_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Rpk_Segment
{
    Rpk_Segment *next;
    Rpk_Segment *prev;
    u32 x, y, w;
};

struct Rpk_Context
{
    Arena *arena;
    b32 initted;
    u32 w, h;
    Rpk_Segment *first_free_segment;
    Rpk_Segment *last_free_segment;
    Rpk_Segment *seg_sentinel;
};

struct Rpk_Result
{
    u32 x, y;
    b16 fit;
};

internal Rpk_Segment *rpk_alloc_segment(Rpk_Context *ctx);
internal void rpk_release_segment(Rpk_Context *ctx, Rpk_Segment *segment);
internal void rpk_init_segment(Rpk_Segment *segment, u32 x, u32 y, u32 w);
internal void rpk_init(Rpk_Context *ctx, u32 w, u32 h);
internal int rpk_seg_cmp(void *s1, void *s2);
internal Rpk_Result rpk_do(Rpk_Context *ctx, u32 w, u32 h);


#endif // RTS_RECT_PACK_H
