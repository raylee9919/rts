/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// # Note: [.h]
//
#include "base/rts_base_inc.h"
#include "os/rts_os.h"

#include "rect_pack/rts_rect_pack.h"
#include "font_provider/rts_fp_ds.h"
#include "font_provider/rts_font_provider.h"
#include "font_provider/dwrite_core.h"

// # Note: [.cpp]
//
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"

#include "rect_pack/rts_rect_pack.cpp"
#include "font_provider/rts_fp_ds.cpp"
#include "font_provider/dwrite_core.cpp"

u8 eos = 0;

// # Note: RTS Font Asset Format
//         It is not production-ready format yet.
//         It is not a format spec for the whole file.
//         It is more like a format of font data section in an asset file.
//
//         1. Face metrics.
//                  
//                  ascent    : f32
//                  descent   : f32
//                  lineGap   : f32
//
//         2. Codepoint to Glyph Indices Mapping.
//            One codepoint can have multiple glyph indices. In the first part of the asset, 
//            aforementioned mappings will be specified in the following form (ignore lines starting with //):
//              
//                  codepoint  : u32
//                  glyphCount : u32
//                  glyphIndex : u16
//
//            End this part with character ';' which is a single byte.
//
//             
//             0 as a glyph index is an invalid index.
//
//          3. Metrics for Glyph Indices
//             
//                  glyphIndex       : u32
//                  uvMinX           : f32
//                  uvMinY           : f32
//                  uvMaxX           : f32
//                  uvMaxY           : f32
//                  width            : f32
//                  height           : f32
//                  leftSideBearing  : f32
//                  rightSideBearing : f32
//                  advanceX         : f32
//
//             End this part with character ';' which is a single byte.
//
//          4. Atlas
//             Top down, left to right.
//
//                  width        : f32
//                  height       : f32
//                  texel[0,3]   : RGBA32
//
//             End this part with character ';' which is a single byte.
//

int main(void)
{
    // # Note: Init core.
    //
    {
        os_init();
        thread_init();
    }

    Arena *permanent_arena = arena_alloc();

    // # Note: Gather path.
    //
    Utf8 binary_path = {};
    Utf8 data_path = {};
    {
        Temporary_Arena scratch = scratch_begin();
        {
            binary_path = os->string_from_system_path_kind(scratch.arena, OS_SYSTEM_PATH_KIND_BINARY);
            Utf8 local_data_path = utf8f(scratch.arena, "%S/data", binary_path);
            Utf8 binary_parent_path = utf8_path_chop_last_slash(binary_path);
            Utf8 parent_data_path = utf8f(scratch.arena, "%S/data", binary_parent_path);

            Os_File_Attributes local_data_attr  = os->attributes_from_file_path(local_data_path);
            Os_File_Attributes parent_data_attr = os->attributes_from_file_path(parent_data_path);

            if (local_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
            {
                data_path = utf8_copy(permanent_arena, local_data_path); 
            }
            else if (parent_data_attr.flags == OS_FILE_FLAG_DIRECTORY)
            {
                data_path = utf8_copy(permanent_arena, parent_data_path); 
            }
        }
        scratch_end(scratch);
    }


    // # Todo/Temporary: Change to pure OS calls.
    //
    Utf8 out_path = utf8f(permanent_arena, "%S/font_asset.txt", data_path);
    FILE *file = fopen((const char *)out_path.str, "wb");
    assert(file);


    // # Note: alloc/init atlas and rect packing context.
    //
    Font_Atlas *atlas;
    {
        Arena *arena = arena_alloc();
        atlas = push_struct(arena, Font_Atlas);
        atlas->arena = arena;

        atlas->width  = 1024;
        atlas->height = 1024;
        atlas->pitch  = (atlas->width << 2);
        atlas->data   = (u8 *)push_size(atlas->arena, atlas->pitch * atlas->height);

        atlas->rpk_ctx = push_struct(atlas->arena, Rpk_Context);
        rpk_init(atlas->rpk_ctx, atlas->arena, atlas->width, atlas->height);
    }


    // # Note: Alloc/init DirectWrite
    //
    dwrite_state = dwrite_alloc();
    dwrite_init();


    // # Note: Configs.
    //
    f32 pt_per_em = 16.0f; // aka, font size.


    // # Note: Check if the desired base font family exists.
    //         Don't really need this if one performs a fallback and doesn't care which font will be used.
    //
#if 0
    WCHAR *base_font_family_name = L"Roboto Mono";
    s64 family_index = dwrite_font_family_index_from_name(base_font_family_name);
    assert(family_index != -1);
#endif


    // # Note: Gather UTF16 I need.
    //
#if 1
    WCHAR *text = push_array(permanent_arena, WCHAR, 4096);
    for (u32 codepoint = 32, i = 0; codepoint <= 126; ++codepoint, ++i)
    {
        text[i] = codepoint;
    }
#else
    WCHAR *text = L"안녕하세요, I am Seong Woo Lee.";
#endif
    u64 text_length = wcslen(text);


    // # Note: Lood ttf from disk and create reference.
    //
    Dwrite_Font_File *font_file = dwrite_font_file_alloc_from_path(utf8f(permanent_arena, "%S/input/font/RobotoMono-Regular.ttf", data_path));
    u32 index = 0; // # Todo: Later on, the user might want to pick desired face index in ttc.

    // # Note: Get face COM from file COM.
    //
    IDWriteFontFace5 *face5 = dwrite_face5_from_font_file_and_index(font_file, index);

    // # Note: Obtain codepoints from the UTF16 text.
    //
    u32 codepoints[4096];
    u32 codepoint_count = 0;

    u16 *ptr = (u16 *)text;
    u16 *opl = ptr + text_length;
    Unicode_Decode consume;
    for (;ptr < opl; ptr += consume.inc)
    {
        consume = utf16_decode(ptr, opl - ptr);
        u32 codepoint = consume.codepoint;
        assert(codepoint_count < array_count(codepoints));
        codepoints[codepoint_count++] = codepoint;
    }

    // # Note: Part1
    //
    DWRITE_FONT_METRICS face_metrics = {};
    face5->GetMetrics(&face_metrics);

    f32 du_per_em = face_metrics.designUnitsPerEm;
    f32 em_per_du = 1.0f / (f32)du_per_em;
    f32 px_per_pt = dwrite_state->px_per_inch / 72.0f;
    f32 px_per_em = pt_per_em * px_per_pt;
    f32 px_per_du = px_per_em * em_per_du;
    //f32 linespace =  (f32)(face_metrics.ascent + face_metrics.descent + face_metrics.lineGap) * px_per_du;
    f32 ascent = face_metrics.ascent * px_per_du;
    f32 descent = face_metrics.descent * px_per_du;
    f32 linegap = face_metrics.lineGap * px_per_du;
    fwrite(&ascent,  sizeof(f32), 1, file);
    fwrite(&descent, sizeof(f32), 1, file);
    fwrite(&linegap, sizeof(f32), 1, file);


    // # Note: For the specific font face, get the mapped glyph indices per codepoints 
    //         we retrieved from the text.
    //
    for (u32 i = 0; i < codepoint_count; i += 1)
    {
        u32 codepoint = codepoints[i];
        // # Todo: It is not proof to redundant mapping yet. Thus, one must specify codepoints with caution.
        //
        Dwrite_Glyph_Indices indices = dwrite_glyph_indices_from_codepoint(permanent_arena, face5, codepoint);
        if (indices.index_count > 0)
        {
            // # Note: Part2
            //
            fwrite(&codepoint, sizeof(u32), 1, file);
            fwrite(&indices.index_count, sizeof(u32), 1, file);
            for (u32 j = 0; j < indices.index_count; ++j)
            {
                fwrite(&indices.indices[j], sizeof(u16), 1, file);
            }
        }
    }
    fwrite(&eos, sizeof(u8), 1, file);


    //Dwrite_Runs *runs = dwrite_map_text_to_glyphs(dwrite_state->font_fallback1, dwrite_state->font_collection, dwrite_state->locale, base_font_family_name, pt_per_em, text, text_length);
    Dwrite_Run_Series *run_series = dwrite_runs_from_text(face5, pt_per_em, text, text_length);

    Glyph_Cel_List *cel_list = push_struct(permanent_arena, Glyph_Cel_List);

    for (Dwrite_Run *run_wrapper = run_series->run_first;
         run_wrapper != NULL;
         run_wrapper = run_wrapper->next)
    {
        DWRITE_GLYPH_RUN run = run_wrapper->e;
        IDWriteFontFace5 *font_face = (IDWriteFontFace5 *)run.fontFace;

        Dwrite_Font_Table_Entry *font_entry = dwrite_get_entry_from_font_table(font_face);
        assert(font_entry);
        Dwrite_Font_Metrics font_metrics = font_entry->metrics;

        // # Note: Pack glyphs in a run into atlas.
        //
        dwrite_atlas_pack(permanent_arena, run_wrapper, &font_entry->glyph_table, atlas, cel_list);

        u32 idx = 0;
        Glyph_Cel *cel = cel_list->first;

        for (; idx < run.glyphCount; idx += 1, cel = cel->next)
        {
            // # Note: Part3
            //
            fwrite(&cel->glyph_index,       sizeof(u32), 1, file);
            fwrite(&cel->uv_min.x,          sizeof(f32), 1, file);
            fwrite(&cel->uv_min.y,          sizeof(f32), 1, file);
            fwrite(&cel->uv_max.x,          sizeof(f32), 1, file);
            fwrite(&cel->uv_max.y,          sizeof(f32), 1, file);
            fwrite(&cel->width_px,          sizeof(f32), 1, file);
            fwrite(&cel->height_px,         sizeof(f32), 1, file);
            fwrite(&cel->offset_px.x,       sizeof(f32), 1, file);
            fwrite(&cel->offset_px.y,       sizeof(f32), 1, file);
            fwrite(&run.glyphAdvances[idx], sizeof(f32), 1, file);
        }
    }
    fwrite(&eos, sizeof(u8), 1, file);

    // # Note: Part4
    //
    fwrite(&atlas->width, sizeof(u32), 1, file);
    fwrite(&atlas->height, sizeof(u32), 1, file);
    fwrite(atlas->data, atlas->pitch * atlas->height, 1, file);
    fwrite(&eos, sizeof(u8), 1, file);
    fclose(file);

    return 0;
}
