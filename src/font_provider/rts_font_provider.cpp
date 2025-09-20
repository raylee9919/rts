/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// ---------------------------------
// @Note: [.h]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_math.h"

#include "rect_pack/rts_rect_pack.h"
#include "font_provider/rts_fp_ds.h"
#include "font_provider/rts_font_provider.h"
#include "font_provider/rts_dwrite.h"

// ---------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"
#include "rts_math.cpp"

#include "rect_pack/rts_rect_pack.cpp"
#include "font_provider/rts_fp_ds.cpp"
#include "font_provider/rts_dwrite.cpp"

int main(void)
{
    // -------------------------------
    // @Note: init.
    {
        os_init();
        thread_init();
    }

    Arena *permanent_arena = arena_alloc();

    dwrite_init();

    // -------------------------------
    // @Note: configs.
    f32 pt_per_em = 20.0f; // aka, font size.
    f32 px_per_inch = 96.0f;
    WCHAR *base_font_family_name = L"Roboto Mono";
    Dwrite_Get_Base_Font_Family_Index_Result family = dwrite_get_base_font_family_index(base_font_family_name);
    Assert(family.exists);
    b32 is_cleartype = TRUE;
    WCHAR *text = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    u64 text_length = wcslen(text);


    // -------------------------------
    // @Note: alloc/init atlas and rect packing context.
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

    for (Dwrite_Run *run_wrapper = unit->run_first; run_wrapper != 0; run_wrapper = run_wrapper->next)
    {
        DWRITE_GLYPH_RUN run = run_wrapper->e;
        IDWriteFontFace5 *font_face = (IDWriteFontFace5 *)run.fontFace;

        Dwrite_Font_Table_Entry *font_entry = dwrite_get_entry_from_font_table(font_face);
        Assert(font_entry);
        Dwrite_Font_Metrics font_metrics = font_entry->metrics;

        // @Note: Create rendering mode of a font face.
        DWRITE_RENDERING_MODE1 rendering_mode = DWRITE_RENDERING_MODE1_NATURAL;
        DWRITE_MEASURING_MODE measuring_mode  = DWRITE_MEASURING_MODE_NATURAL;
        DWRITE_GRID_FIT_MODE grid_fit_mode    = DWRITE_GRID_FIT_MODE_DEFAULT;

        Assert(SUCCEEDED(font_face->GetRecommendedRenderingMode(run.fontEmSize,
                                                                px_per_inch, px_per_inch,
                                                                NULL, // transform
                                                                run.isSideways,
                                                                DWRITE_OUTLINE_THRESHOLD_ANTIALIASED,
                                                                measuring_mode,
                                                                dwrite.rendering_params,
                                                                &rendering_mode,
                                                                &grid_fit_mode)));


        dwrite_pack_glyphs_in_run_to_atlas(is_cleartype, run_wrapper,
                                           rendering_mode, measuring_mode, grid_fit_mode,
                                           &font_entry->glyph_table, atlas, &glyph_cels);
    }

    return 0;
}
