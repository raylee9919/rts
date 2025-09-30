#ifndef RTS_FONT_H
#define RTS_FONT_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// # Note: Glyph Metrics
//
struct Glyph_Metrics
{
    Glyph_Metrics *next;
    Glyph_Metrics *prev;
    Glyph_Metrics *first;
    Glyph_Metrics *last;

    u32 glyph_index;

    f32 uv_min_x;
    f32 uv_min_y;
    f32 uv_max_x;
    f32 uv_max_y;
    f32 width;
    f32 height;
    f32 left_side_bearing;
    f32 top_side_bearing;
    f32 advance_x;
};


// # Note: CMAP
//         Technically, CMAP is a mapping to glyph indices of a face from           
//         Unicodes, etc.. But we are using the term as a mapping from 
//         codepoint to glyph indices.
//
struct Face_Cmap
{
    Face_Cmap *next;
    Face_Cmap *prev;
    Face_Cmap *first;
    Face_Cmap *last;

    u32 codepoint;

    u16 *indices;
    u64 index_count;
};

// # Note: Font Face
//
struct Face
{
    Arena *arena;

    f32 linespace;

    // # Note: Glyph index to it's metrics mapping.
    Glyph_Metrics *metrics_table;
    u64 metrics_table_size;

    // # Note: CMAP
    Face_Cmap *cmap_table;
    u64 cmap_table_size;

    // # Note: Atlas
    //         Speced to be R8G8B8A8 format currently.
    void *data;
    u32 width;
    u32 height;
};


// # Note: Function declarations.
//
internal Face *face_alloc(void);

// # Note: Glyph metrics.
internal u64 glyph_metrics_slot_from_index(Face *face, u16 glyph_index);
internal Glyph_Metrics * glyph_metrics_get(Face *face, u16 glyph_index);
internal void glyph_metrics_put(Face *face, Glyph_Metrics metrics);


// # Note: CMAP
internal u64 face_cmap_slot_from_codepoint(Face *face, u32 codepoint);
internal Face_Cmap *face_cmap_get(Face *face, u32 codepoint);
internal void face_cmap_put(Face *face, u32 codepoint, u16 *indices, u64 index_count);
internal u64 face_cmap_glyph_count_from_codepoint(Face *face, u32 codepoint);
internal u16 *face_cmap_glyph_indices_from_codepoint(Face *face, u32 codepoint);


#endif // RTS_FONT_H
