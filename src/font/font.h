// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_FONT_H
#define RTS_FONT_H

#include "basic/core.h"
#include "basic/array.h"
#include "basic/hash_table.h"
#include "basic/string.h"
#include "os/os.h"

struct hb_face_t;
struct hb_font_t;
struct hb_raster_draw_t;
struct hb_raster_paint_t;

struct Glyph_Cache {
    vec2 min_uv;
    vec2 max_uv;

    // Rasterized glyph's information.
    // top and left are added to the given pen position.
    s32 top;
    s32 left;
    s32 width;
    s32 height;
};

struct Font {
    hb_face_t         *face;
    hb_font_t         *font;
    hb_raster_draw_t  *raster;
    hb_raster_paint_t *paint;
    f32                points_per_em;
    s32                ascender;
    s32                descender;
    s32                line_gap;

    u32                codepoint_for_unknown = 0xfffd; // 'REPLACEMENT CHARACTER'

    Table<u32, Glyph_Cache> glyph_cache; // from glyph ID
};

Pair<b32, Font> 
font_create(u8 *data, s64 size, s32 font_size);

Pair<b32, Font> 
font_create_from_file(String file, s32 font_size);

void 
font_destroy(Font *font);

void
draw_string(String str, 
            Font  *font,
            vec4   color);


#endif // RTS_FONT_H
