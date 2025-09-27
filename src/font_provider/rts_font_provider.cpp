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
#include "font_provider/rts_dwrite.h"

// # Note: [.cpp]
//
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"

#include "rect_pack/rts_rect_pack.cpp"
#include "font_provider/rts_fp_ds.cpp"
#include "font_provider/rts_dwrite.cpp"

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

    dwrite_init();

    // # Note: configs.
    //
    f32 pt_per_em   = 40.0f; // aka, font size.
    f32 px_per_inch = 96.0f;
    WCHAR *base_font_family_name = L"Roboto Mono";
    Dwrite_Get_Base_Font_Family_Index_Result family = dwrite_get_base_font_family_index(base_font_family_name);
    assert(family.exists);
    b32 is_cleartype = TRUE;

    // # Note: Gather utf16 I need.
    //
    WCHAR *text = push_array(permanent_arena, WCHAR, 4096);
    for (u32 codepoint = 32, i = 0; codepoint <= 126; ++codepoint, ++i)
    {
        text[i] = codepoint;
    }
    u64 text_length = wcslen(text);


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

    Dwrite_Unit *unit = dwrite_map_text_to_glyphs(dwrite.font_fallback1, dwrite.font_collection, dwrite.text_analyzer1, dwrite.locale, base_font_family_name, pt_per_em, px_per_inch, text, text_length);

    Glyph_Cel_Array glyph_cels = {};
    dar_init(&glyph_cels, permanent_arena);

    for (Dwrite_Run *run_wrapper = unit->run_first;
         run_wrapper != NULL;
         run_wrapper = run_wrapper->next)
    {
        DWRITE_GLYPH_RUN run = run_wrapper->e;
        IDWriteFontFace5 *font_face = (IDWriteFontFace5 *)run.fontFace;

        Dwrite_Font_Table_Entry *font_entry = dwrite_get_entry_from_font_table(font_face);
        assert(font_entry);
        Dwrite_Font_Metrics font_metrics = font_entry->metrics;

        // # Note: Create rendering mode of a font face.
        //
        DWRITE_RENDERING_MODE1 rendering_mode = DWRITE_RENDERING_MODE1_NATURAL;
        DWRITE_MEASURING_MODE measuring_mode  = DWRITE_MEASURING_MODE_NATURAL;
        DWRITE_GRID_FIT_MODE grid_fit_mode    = DWRITE_GRID_FIT_MODE_DEFAULT;

        assert(SUCCEEDED(font_face->GetRecommendedRenderingMode(run.fontEmSize,
                                                                px_per_inch, px_per_inch,
                                                                NULL, // transform
                                                                run.isSideways,
                                                                DWRITE_OUTLINE_THRESHOLD_ANTIALIASED,
                                                                measuring_mode,
                                                                dwrite.rendering_params,
                                                                &rendering_mode,
                                                                &grid_fit_mode)));


        // # Note: Pack glyph into atlas.
        //
        dwrite_pack_glyphs_in_run_to_atlas(is_cleartype, run_wrapper,
                                           rendering_mode, measuring_mode, grid_fit_mode,
                                           &font_entry->glyph_table, atlas, &glyph_cels);
    }

    // # Todo/Temporary: Change to pure OS calls.
    //
    FILE *file = fopen("font_atlas.temp", "wb");
    if (file)
    {
        fwrite(atlas->data, atlas->pitch * atlas->height, 1, file);
        fclose(file);
    }

    u16 *ptr = (u16 *)text;
    u16 *opl = ptr + text_length;
    Unicode_Decode consume;
    for (;ptr < opl; ptr += consume.inc)
    {
        consume = utf16_decode(ptr, opl - ptr);
        u32 codepoint = consume.codepoint;
    }



    return 0;
}
