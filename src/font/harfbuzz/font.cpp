// Copyright Seong Woo Lee. All Rights Reserved.

#include "font/font.h"
#include "basic/log.h"
#include "os/os.h"
#include "font/font.h"
#include "gfx/gfx.h"
#include "rect_pack/rect_pack.h"
#include "third_party/harfbuzz/hb.h"
#include "third_party/harfbuzz/hb-raster.h"

// @Temporary
#include "renderer/immediate.h"

struct Font_System {
    b32         initted;
    b32         atlas_dirty;
    s32         atlas_size;
    Guid        atlas;
    u8         *atlas_data;
    Rpk_Context rect_pack_context;
};

struct Paragraph {
    String string;
    int    newline_count_afterward;
};

struct Shaped_Glyph {
    u32 id;    // id in the font
    s32 origin_x;
    s32 origin_y;
};

static Font_System _font_system;

static Font_System *
get_font_system()
{
    if (!_font_system.initted) {
        _font_system.atlas = guid_generate();

        s32 sz = 1024; // @Temporary
        _font_system.atlas_size = sz;

        RHI_Texture_Desc desc = {};
        desc.name       = S("Glyph Atlas");
        desc.type       = RHI_TEXTURE_TYPE_2D;
        desc.format     = RHI_FORMAT_RGBA8_UNORM; 
        desc.usage      = RHI_TEXTURE_USAGE_SAMPLED;
        desc.width      = sz;
        desc.height     = sz;
        desc.mip_levels = 1;
        desc.depth      = 1;
        gfx_texture_create(_font_system.atlas, desc);

        // @Temporary
        Arena *arena = arena_alloc();
        rpk_init(&_font_system.rect_pack_context, 
                 arena,
                 _font_system.atlas_size,
                 _font_system.atlas_size);

        _font_system.atlas_data = push_array(arena, u8, 4*sz*sz);

        _font_system.initted = true;
    }

    return &_font_system;
}

Pair<b32, Font> 
font_create(u8 *data, s64 size, s32 font_size)
{
    // Create hb blob
    const char *data_c = (const char*)data;
    hb_blob_t *blob = hb_blob_create_or_fail(data_c, size, HB_MEMORY_MODE_DUPLICATE, NULL, NULL);
    if (!blob) {
        log_error(S("hb_blob_create_or_fail() failed."));
        return {false, {}};
    }

    // Create hb face
    hb_face_t *face = hb_face_create_or_fail(blob, 0);

    hb_blob_destroy(blob); // no longer needed no matter what.
    blob = NULL;

    if (!face) {
        log_error(S("hb_face_create_or_fail() failed."));
        return {false, {}};
    }

    // Create hb font
    hb_font_t *font = hb_font_create(face);
    hb_font_set_scale(font, font_size, font_size); // @Todo: wtf does it actually do??

    float pt_per_em = hb_font_get_ptem(font);

    hb_font_extents_t extents;
    if (!hb_font_get_h_extents(font, &extents)) {
        hb_font_destroy(font);
        hb_face_destroy(face);
        return {false, {}};
    }


    // Create raster object
    hb_raster_draw_t *raster = hb_raster_draw_create_or_fail();
    if ( !raster ){
        hb_font_destroy(font);
        hb_face_destroy(face);
        return {false, {}};
    }

    // Create paint object
    hb_raster_paint_t *paint = hb_raster_paint_create_or_fail();
    if ( !paint ){
        hb_raster_draw_destroy(raster);
        hb_font_destroy(font);
        hb_face_destroy(face);
        return {false, {}};
    }


    Font result;
    result.face               = face;
    result.font               = font;
    result.raster             = raster;
    result.paint              = paint;
    result.points_per_em      = pt_per_em;
    result.ascender           = extents.ascender;
    result.descender          = extents.descender;
    result.line_gap           = extents.line_gap;

    // @Temporary Allocator
    result.glyph_cache.allocator = {crt_proc, nullptr};

    return {true, result};
}

Pair<b32, Font> 
font_create_from_file(String file, s32 font_size)
{
    String s = read_entire_file(file, tctx.temp);
    if (s) {
        return font_create(s.str, s.len, font_size);
    }
    return {false, {}};
}

void 
font_destroy(Font *font)
{
    hb_raster_paint_destroy(font->paint);
    hb_raster_draw_destroy(font->raster);
    hb_font_destroy(font->font);
    hb_face_destroy(font->face);
    memset(font, 0, sizeof(*font));
}

static Array<Paragraph>
break_by_paragraphs(String s, Allocator allocator)
{
    Array<Paragraph> result = {};
    result.allocator = allocator;

    s64 cursor = 0;
    while (cursor < s.len) {
        s64 end = find_index_from_left(s, '\n', cursor);
        if (end == -1) end = s.len;

        s64 text_end = end;
        if (text_end > cursor && s.str[text_end - 1] == '\r') text_end -= 1;

        Paragraph p = {};
        p.string.str = s.str + cursor;
        p.string.len = text_end - cursor;

        cursor = end;
        while (cursor < s.len && (s.str[cursor] == '\n' || s.str[cursor] == '\r')) {
            if (s.str[cursor] == '\n') p.newline_count_afterward += 1;
            cursor += 1;
        }

        array_add(&result, p);
    }
    return result;
}

/*
  str:          string to shape
  font:         font to use
  langauge:     BCP 47 language tag
  allocator:    allocates shaped glyphs into the array
  _pen_x:       initial x coordinate of the pen
  _pen_y:       initial y coordinate of the pen
*/
static Array<Shaped_Glyph>
shape_string(String     str, 
             Font      *font, 
             String     language, 
             Allocator  allocator,
             s32        _pen_x,
             s32        _pen_y)
{
    s32 pen_x = _pen_x;
    s32 pen_y = _pen_y;

    Array<Shaped_Glyph> result = {};
    result.allocator = allocator;

    hb_buffer_t *buf = hb_buffer_create();

    Array<Paragraph> paragraphs = break_by_paragraphs(str, tctx.temp);
    for (Paragraph &paragraph : paragraphs) 
    {
        hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
        hb_buffer_set_script(buf, HB_SCRIPT_LATIN); // @Temporary
        hb_buffer_set_language(buf, hb_language_from_string((const char*)language.str, (int)language.len));
        hb_buffer_set_replacement_codepoint(buf, font->codepoint_for_unknown);

        const char *string_c = (const char *)paragraph.string.str;
        int len = paragraph.string.len;
        hb_buffer_add_utf8(buf, string_c, len, 0, len);

        hb_shape(font->font, buf, NULL, 0);

        unsigned int count;
        hb_glyph_info_t *info = hb_buffer_get_glyph_infos(buf, &count);
        hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buf, &count);

        for (unsigned int i = 0; i < count; ++i) 
        {
            u32 glyph_id  = info[i].codepoint; // it's glyph index after shaping!
            s32 x_advance = positions[i].x_advance;
            s32 y_advance = positions[i].y_advance;
            s32 x_offset  = positions[i].x_offset;
            s32 y_offset  = positions[i].y_offset;

            hb_glyph_extents_t ext;
            R_ASSERT( hb_font_get_glyph_extents(font->font, glyph_id, &ext) );

            // Skip whitespace
            if (ext.width > 0 && -ext.height > 0) {
                Shaped_Glyph glyph;
                glyph.id       = glyph_id;
                glyph.origin_x = pen_x + x_offset;
                glyph.origin_y = pen_y - y_offset;
                array_add(&result, glyph);
            }

            pen_x += x_advance;
            pen_y -= y_advance;
        }

        // Advance line(s)
        s32 d = (-font->descender + font->ascender + font->line_gap);
        pen_x = _pen_x;
        pen_y += paragraph.newline_count_afterward * d;

        hb_buffer_clear_contents(buf);
    }

    hb_buffer_destroy(buf);

    return result;
}

static void
raster_glyph(Font *font,
             hb_codepoint_t glyph_id)
{
    Font_System *sys = get_font_system();

    R_ASSERT ( hb_raster_draw_glyph_or_fail(font->raster, font->font, glyph_id) );
    hb_raster_image_t *image = hb_raster_draw_render(font->raster);

    hb_raster_extents_t extents;
    hb_raster_image_get_extents(image, &extents);

    if (extents.width > 0 && extents.height > 0) 
    {
        Rpk_Result pack = rpk_pack(&sys->rect_pack_context, extents.width, extents.height);
        if (pack.did_fit)
        {
            u8 *data    = (u8*)hb_raster_image_get_buffer(image);
            u8 *end_row = data + extents.stride * (extents.height - 1); // scan starts from the bottom row.

            for (u32 row = 0; row < extents.height; ++row) 
            {
                for (u32 col = 0; col < extents.width; ++col) 
                {
                    u32 *dst = (u32*)sys->atlas_data + sys->atlas_size*(pack.y + row) + (pack.x + col);
                    u8 c = *(end_row - row*extents.stride + col);
                    *dst = 0x00ffffff | (c<<24);
                }
            }

            // Add new cache entry
            f32 rcp_atlas_size = 1.f / (f32)sys->atlas_size;
            f32 dU = (f32)extents.width  * rcp_atlas_size;
            f32 dV = (f32)extents.height * rcp_atlas_size;
            Glyph_Cache new_cache;
            new_cache.min_uv = vec2((f32)pack.x * rcp_atlas_size, (f32)pack.y * rcp_atlas_size);
            new_cache.max_uv = new_cache.min_uv + vec2(dU, dV);
            new_cache.top    = extents.y_origin + (s32)extents.height;
            new_cache.left   = extents.x_origin;
            new_cache.width  = extents.width;
            new_cache.height = extents.height;
            table_add(&font->glyph_cache, glyph_id, new_cache);

            // Mark dirty
            sys->atlas_dirty = true;
        }
        else
        {
            // @Todo: LRU Cache eviction?
            R_ASSERT(!"Coudln't fit in the atlas!");
        }
    }
    else
    {
        table_add(&font->glyph_cache, glyph_id, Glyph_Cache{});
    }

    hb_raster_image_destroy(image);
}

static void
upload_atlas_if_dirty()
{
    Font_System *sys = get_font_system();
    if (sys->atlas_dirty)
    {
        gfx_texture_upload(sys->atlas, 
                           RHI_FORMAT_RGBA8_UNORM, 
                           get_font_system()->atlas_data, 
                           sys->atlas_size*sys->atlas_size*4,
                           sys->atlas_size,
                           sys->atlas_size);
        sys->atlas_dirty = false;
    }
}

/*
  str:   string to draw
  font:  font to use
  color: color of the string
*/
Pair<s32, s32>
draw_text(String str, 
          Font  *font,
          vec4   color)
{
    Font_System *sys = get_font_system();

    s32 pen_x = 0, pen_y = 80;

    // @Fix: It skips whitespaces at the end of each line.
    s32 min_x = INT32_MAX;
    s32 max_x = INT32_MIN;
    s32 min_y = INT32_MAX;
    s32 max_y = INT32_MIN;

    Array<Shaped_Glyph> glyphs = shape_string(str, font, S("en"), tctx.temp, pen_x, pen_y);  // @Todo: script automation

    for (Shaped_Glyph& glyph : glyphs) 
    {
        // Find glyph bitmap cache, and if doesn't exist, rasterize new one
        Glyph_Cache *cache = table_find_pointer(&font->glyph_cache, glyph.id);
        if (!cache) raster_glyph(font, glyph.id);
        cache = table_find_pointer(&font->glyph_cache, glyph.id);
        R_ASSERT(cache);


        // Skip empty bitmaps
        if (cache->width <= 0 || cache->height <= 0) 
            continue;


        // Draw
        vec2 uv0 = cache->min_uv;
        vec2 uv1 = vec2(cache->min_uv.x, cache->max_uv.y);
        vec2 uv2 = vec2(cache->max_uv.x, cache->min_uv.y);
        vec2 uv3 = cache->max_uv;
        s32 x = glyph.origin_x + cache->left;
        s32 y = glyph.origin_y - cache->top;
        s32 w = cache->width;
        s32 h = cache->height;
        draw_quad(x, y, w, h,
                  uv0, uv1, uv2, uv3, 
                  color, color, color, color,
                  sys->atlas);

        min_x = min(min_x, x);
        min_y = min(min_y, y);
        max_x = max(max_x, x + w);
        max_y = max(max_y, y + h);
    }
    
    upload_atlas_if_dirty();

    if (min_x > max_x) return { 0, 0 };
    else               return { max_x - min_x, max_y - min_y };
}
