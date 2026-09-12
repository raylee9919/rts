// Copyright Seong Woo Lee. All Rights Reserved.

//
// Skyline-based rectangle packing.
//

#ifndef RTS_RECT_PACK_H
#define RTS_RECT_PACK_H

#include "basic/core.h"
#include "basic/arena.h"

struct Rpk_Segment {
    Rpk_Segment *next;
    Rpk_Segment *prev;
    u32 x, y, w;
};

struct Rpk_Context {
    Arena *arena;
    b32 initted;
    u32 w, h;
    Rpk_Segment *first_free_segment;
    Rpk_Segment *last_free_segment;
    Rpk_Segment *segment_first;
    Rpk_Segment *segment_last;
};

struct Rpk_Result {
    u32 x, y;
    b16 did_fit;
};

//
// API
//
void         rpk_init(Rpk_Context *ctx, Arena *arena, u32 w, u32 h);
Rpk_Result   rpk_do(Rpk_Context *ctx, u32 w, u32 h);


#endif // RTS_RECT_PACK_H
