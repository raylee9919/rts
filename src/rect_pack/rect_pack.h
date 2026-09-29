// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RECT_PACK_H
#define RTS_RECT_PACK_H

//
// Skyline-based rectangle packing.
//

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
    b32 did_fit;
};


void 
rpk_init(Rpk_Context *ctx, 
         Arena       *arena, 
         u32          width, 
         u32          height);

Rpk_Result 
rpk_pack(Rpk_Context *ctx, 
         u32          width, 
         u32          height);


#endif // RTS_RECT_PACK_H
