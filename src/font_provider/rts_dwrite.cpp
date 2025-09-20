/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

// ---------------------------------
// @Note: 2nd Tier Glyph Hash Table
internal u64
dwrite_hash_glyph_index(u16 idx)
{
    // @Todo: Better hash
    u64 result = (u64)idx * 16;
    return result;
}

internal Dwrite_Glyph_Table_Entry *
dwrite_get_glyph_entry_from_table(Dwrite_Glyph_Table glyph_table, u16 glyph_index)
{
    Dwrite_Glyph_Table_Entry *result = NULL;

    u64 hashed = dwrite_hash_glyph_index(glyph_index);
    u64 entry_count = glyph_table.entry_count;
    u64 home_position = (hashed % entry_count);

    for (u64 i = 0; i < entry_count; ++i)
    {
        u64 idx = (home_position + i) % entry_count;

        if (glyph_table.entries[idx].occupied)
        { 
            if (glyph_table.entries[idx].idx == glyph_index)
            {
                result = glyph_table.entries + idx;
                break;
            }
        }
    }

    return result;
}

internal void
dwrite_insert_glyph_cel_to_table(Dwrite_Glyph_Table *glyph_table, u16 glyph_index, Glyph_Cel cel)
{
    u64 hashed = dwrite_hash_glyph_index(glyph_index);
    u64 entry_count = glyph_table->entry_count;
    u64 home_position = (hashed % entry_count);

    u64 idx_to_insert = home_position;
    b32 found_place_to_insert = false;
    b32 exists_already = false;

    for (u64 i = 0; i < entry_count; ++i)
    {
        u64 idx = (home_position + i) % entry_count;

        if (glyph_table->entries[idx].occupied)
        { 
            if (glyph_table->entries[idx].idx == glyph_index)
            {
                exists_already = true;
                break;
            }
        }
        else if (! found_place_to_insert)
        {
            idx_to_insert = idx;
            found_place_to_insert = true;
        }
    }

    if (! exists_already)
    {
        if (found_place_to_insert)
        {
            Dwrite_Glyph_Table_Entry *entry = glyph_table->entries + idx_to_insert;
            entry->idx      = glyph_index;
            entry->cel      = cel;
            entry->occupied = true;
        }
        else
        {
            Assert(! "Insufficient hash table entries.");
        }
    }
}

// ---------------------------------
// @Note: FontFace Hash Table (Outer Hash Table)
internal u64
dwrite_hash_font(IDWriteFontFace *key)
{
    // @Todo: Better hash
    u64 result = int_from_ptr(key) + 1234;
    return result;
}

internal Dwrite_Font_Table_Entry *
dwrite_get_entry_from_font_table(IDWriteFontFace *font_face)
{
    Dwrite_Font_Table_Entry *result = NULL;

    u64 hashed = dwrite_hash_font(font_face);
    u64 entry_count = dwrite.font_table.entry_count;
    u64 home_position = (hashed % entry_count);

    for (u64 i = 0; i < entry_count; ++i)
    {
        u64 idx = (home_position + i) % entry_count;

        if (dwrite.font_table.entries[idx].occupied)
        { 
            if (dwrite.font_table.entries[idx].key == font_face)
            {
                result = dwrite.font_table.entries + idx;
                break;
            }
        }
    }

    return result;
}

internal void
dwrite_insert_font_to_table(IDWriteFontFace *font_face, Dwrite_Font_Metrics metrics)
{
    u64 hashed = dwrite_hash_font(font_face);
    u64 entry_count = dwrite.font_table.entry_count;
    u64 home_position = (hashed % entry_count);

    u64 idx_to_insert = home_position;
    b32 found_place_to_insert = false;
    b32 exists_already = false;

    for (u64 i = 0; i < entry_count; ++i)
    {
        u64 idx = (home_position + i) % entry_count;

        if (dwrite.font_table.entries[idx].occupied)
        { 
            if (dwrite.font_table.entries[idx].key == font_face)
            {
                exists_already = true;
                break;
            }
        }
        else if (! found_place_to_insert)
        {
            idx_to_insert = idx;
            found_place_to_insert = true;
        }
    }

    if (! exists_already)
    {
        if (found_place_to_insert)
        {
            Dwrite_Font_Table_Entry *entry = dwrite.font_table.entries + idx_to_insert;
            entry->key      = font_face;
            entry->metrics  = metrics;
            entry->glyph_table.entry_count = 256;
            entry->glyph_table.entries = push_array(dwrite.arena, Dwrite_Glyph_Table_Entry, entry->glyph_table.entry_count); // @Todo: Proper arena
            entry->occupied = true;
        }
        else
        {
            Assert(! "Insufficient hash table entries.");
        }
    }
}

// @Note: Determines the longest run of characters that map 1:1 to glyphs without
// ambiguity. In that case, it returns TRUE and you can immediately use indices.
// Otherwise, perform full glyph shaping.
internal Dwrite_Map_Complexity_Result
dwrite_map_complexity(Arena *arena,
                      IDWriteTextAnalyzer1 *text_analyzer,
                      IDWriteFontFace *font_face,
                      WCHAR *text, u32 text_length)
{
    Dwrite_Map_Complexity_Result result = {};

    b32 is_simple;
    u32 mapped_length;
    u32 index_cap = text_length;
    u16 *_indices = push_array(arena, u16, index_cap);

    HRESULT hr = text_analyzer->GetTextComplexity(text, text_length, font_face,
                                                  /* out */
                                                  &is_simple, &mapped_length, _indices);
    Assert(SUCCEEDED(hr));

    result.glyph_indices = _indices;
    result.is_simple     = is_simple;
    result.mapped_length = mapped_length;

    return result;
}

internal Dwrite_Font_Fallback_Result
dwrite_font_fallback(IDWriteFontFallback1 *font_fallback,
                     IDWriteFontCollection *font_collection,
                     WCHAR *base_family, WCHAR *locale,
                     WCHAR *text, u32 text_length)
{
    Dwrite_Font_Fallback_Result result = {};

    // @Note: It's safe to ignore scale in practice. -lhecker
    FLOAT dummy_scale;

    Dwrite_Text_Analysis_Source src = {locale, text, text_length};
    font_fallback->MapCharacters(&src, 0/*offset*/, text_length, font_collection, base_family,
                                 NULL/*fontAxisValues*/, 0/*fontAxisValueCount*/,
                                 /* out */
                                 &result.length, &dummy_scale, &result.font_face);

    // @Todo: If no font contains the given codepoints MapCharacters() will return a NULL font_face.
    // We need to replace them with ? glyphs, which this code doesn't do yet (by convention that's glyph index 0 in any font).
    Assert(result.font_face);

    return result;
}

internal void
dwrite_abort(WCHAR *message)
{
    os.abort();
}

internal void
dwrite_init(void)
{
    {
        dwrite.arena = arena_alloc();
        dwrite.unit_arena = arena_alloc();
        dwrite.font_table.entry_count = 32;
        dwrite.font_table.entries = push_array(dwrite.arena, Dwrite_Font_Table_Entry, dwrite.font_table.entry_count);
    }

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(dwrite.factory), (IUnknown **)&dwrite.factory)))
    { dwrite_abort(L"DWriteCreateFactory() Error."); }

    if (FAILED(dwrite.factory->GetSystemFontCollection(&dwrite.font_collection)))
    { dwrite_abort(L"GetSystemFontCollection() Error."); }

    if (FAILED(dwrite.factory->GetSystemFontFallback(&dwrite.font_fallback)))
    { dwrite_abort(L"GetSystemFontFallback() Error."); }

    if (FAILED(dwrite.font_fallback->QueryInterface(__uuidof(dwrite.font_fallback1), (void **)&dwrite.font_fallback1)))
    { dwrite_abort(L"Error while querying IDWriteFontFallback1 interface."); }

    if (FAILED(dwrite.factory->CreateTextAnalyzer(&dwrite.text_analyzer)))
    { dwrite_abort(L"CreateTextAnalyzer() Error."); }

    if (FAILED(dwrite.text_analyzer->QueryInterface(__uuidof(dwrite.text_analyzer1), (void **)&dwrite.text_analyzer1)))
    { dwrite_abort(L"Error while querying IDWriteTextAnalyzer1 interface."); }


    // set locale.
    wchar_t *default_locale = L"en-US";
    if (! GetUserDefaultLocaleName(dwrite.locale, array_count(dwrite.locale)))
    { memory_copy(dwrite.locale, default_locale, sizeof(default_locale)); }

    // create rendering parameters.
    if (FAILED(dwrite.factory->CreateRenderingParams(&dwrite.rendering_params)))
    { dwrite_abort(L"IDWriteFactroy::CreateRenderingParams() Error."); }
}

internal Dwrite_Get_Base_Font_Family_Index_Result
dwrite_get_base_font_family_index(wchar_t *base_font_family_name)
{
    Dwrite_Get_Base_Font_Family_Index_Result result = {};
    if (FAILED(dwrite.font_collection->FindFamilyName(base_font_family_name, &result.index, &result.exists))) 
    { dwrite_abort(L"IDWriteFontCollection::FindFamilyName() Error."); }
    return result;
}

internal void
dwrite_pack_glyphs_in_run_to_atlas(b32 is_cleartype,
                                   Dwrite_Run *run_wrapper,
                                   DWRITE_RENDERING_MODE1 rendering_mode,
                                   DWRITE_MEASURING_MODE measuring_mode,
                                   DWRITE_GRID_FIT_MODE grid_fit_mode,
                                   Dwrite_Glyph_Table *glyph_table,
                                   Font_Atlas *atlas,
                                   Glyph_Cel_Array *glyph_cels)
{
    HRESULT hr = S_OK;
    Temporary_Arena scratch = scratch_begin();

    DWRITE_GLYPH_RUN run = run_wrapper->e;
    u64 glyph_count = (u64)run.glyphCount;;

    IDWriteFontFace5 *font_face = (IDWriteFontFace5 *)run.fontFace;

    Dwrite_Font_Table_Entry *font_entry = dwrite_get_entry_from_font_table(font_face);
    Assert(font_entry);
    Dwrite_Font_Metrics font_metrics = font_entry->metrics;

    DWRITE_TEXTURE_TYPE texture_type = (is_cleartype) ? DWRITE_TEXTURE_CLEARTYPE_3x1 : DWRITE_TEXTURE_ALIASED_1x1;

    // Check if each glyph in the run exists in the inner hash table.
    for (u32 i = 0; i < glyph_count; ++i)
    {
        u16 glyph_index = run.glyphIndices[i];

        Dwrite_Glyph_Table_Entry *entry = dwrite_get_glyph_entry_from_table(*glyph_table, glyph_index);

        if (! entry) // glyph index doesn't exist in the inner-table
        {
            // Get single glyph's metrics.
            DWRITE_GLYPH_METRICS metrics = {};
            Assert(SUCCEEDED(font_face->GetDesignGlyphMetrics(&glyph_index, 1, &metrics, run.isSideways)));

            // CreateGlyphRunAnalysis() doesn't support DWRITE_RENDERING_MODE_OUTLINE.
            // We won't bother big glyphs. (many hundreds of pt)
            if (rendering_mode == DWRITE_RENDERING_MODE1_OUTLINE)
            {
                rendering_mode = DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC; 
            }

            DWRITE_GLYPH_RUN single_glyph_run = {};
            {
                single_glyph_run.fontFace      = font_face;
                single_glyph_run.fontEmSize    = run.fontEmSize;
                single_glyph_run.glyphCount    = 1;
                single_glyph_run.glyphIndices  = &glyph_index;
                single_glyph_run.glyphAdvances = NULL;
                single_glyph_run.glyphOffsets  = NULL;
                single_glyph_run.isSideways    = run.isSideways;
                single_glyph_run.bidiLevel     = run.bidiLevel;
            }

            IDWriteGlyphRunAnalysis *analysis = NULL;
            Assert(SUCCEEDED(dwrite.factory->CreateGlyphRunAnalysis(&single_glyph_run,
                                                                    NULL, // transform
                                                                    rendering_mode,
                                                                    measuring_mode,
                                                                    grid_fit_mode,
                                                                    is_cleartype ? DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE : DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE,
                                                                    0.0f, // baselineOriginX
                                                                    0.0f, // baselineOriginY
                                                                    &analysis)));

            // @Note: GetAlphaTextureBounds() -> RECT exaplanation.
            //
            // bounds.top ------++-----######--+
            //   (-7)           ||  ############
            //                  ||####      ####
            //                  |###       #####
            //  baseline ______ |###      #####|
            //   origin        \|############# |
            //  (= 0,0)         \|###########  |
            //                  ++-------###---+
            //                  ##      ###    |
            // bounds.bottom ---+#########-----+
            //    (+2)          |              |
            //             bounds.left     bounds.right
            //                 (-1)           (+14)
            //

            RECT bounds = {};
            hr = analysis->GetAlphaTextureBounds(texture_type, &bounds);
            if (FAILED(hr))
            {
                // @Todo: The font doesn't support DWRITE_TEXTURE_CLEARTYPE_3x1.
                // Retry with DWRITE_TEXTURE_ALIASED_1x1.
                Assert(! "x");
            }

            Glyph_Cel cel = {};

            if ((bounds.right > bounds.left) && (bounds.bottom > bounds.top))
            {
                u32 blackbox_width  = bounds.right - bounds.left;
                u32 blackbox_height = bounds.bottom - bounds.top;

                u32 margin = 1;
                u32 bitmap_width  = blackbox_width + 2*margin;
                u32 bitmap_height = blackbox_height + 2*margin;

                u32 rgb_bitmap_size = (is_cleartype) ? (blackbox_width*3)*blackbox_height : blackbox_width*blackbox_height; 
                u8 *bitmap_data_rgb = (u8 *)push_size(scratch.arena, rgb_bitmap_size);
                Assert(SUCCEEDED(analysis->CreateAlphaTexture(texture_type, &bounds, bitmap_data_rgb, rgb_bitmap_size)));

                u32 x1 = 0;
                u32 y1 = 0;
                u32 x2 = 0;
                u32 y2 = 0;

                Rpk_Result rpk_result = rpk_do(atlas->rpk_ctx, bitmap_width, bitmap_height);

                if (rpk_result.fit)
                {
                    x1 = rpk_result.x;
                    y1 = rpk_result.y;
                    x2 = x1 + bitmap_width;
                    y2 = y1 + bitmap_height;

                    // RGB to RGBA
                    for (u32 r = 0; r < blackbox_height; ++r)
                    {
                        for (u32 c = 0; c < blackbox_width; ++c)
                        {
                            u8 *dst = atlas->data + (y1+r+margin)*atlas->pitch + (x1+c+margin)*4;
                            u8 *src = bitmap_data_rgb + r*blackbox_width*3 + c*3;
                            *(u32 *)dst = *(u32 *)src;
                            // @Note: Alpha doesn't matter since Cleatype doesn't handle alpha.
                        }
                    }
                }
                else
                {
                    Assert(! "Couldn't fit in the atlas");
                }

                cel.is_empty     = false;
                cel.uv_min       = {(f32)(x1 + margin) / (f32)atlas->width, (f32)(y1 + margin) / (f32)atlas->height};
                cel.uv_max       = {(f32)(x2 - margin) / (f32)atlas->width, (f32)(y2 - margin) / (f32)atlas->height};
                cel.width_px     = (f32)blackbox_width;
                cel.height_px    = (f32)blackbox_height;
                cel.offset_px.x  = (f32)bounds.left;
                cel.offset_px.y  = (f32)-bounds.top;
            }
            else
            {
                cel.is_empty     = true;
                cel.uv_min       = v2{0.0f, 0.0f};
                cel.uv_max       = v2{0.0f, 0.0f};
                cel.width_px     = 0.0f;
                cel.height_px    = 0.0f;
                cel.offset_px.x  = 0.0f;
                cel.offset_px.y  = 0.0f;
            }

            dar_push(glyph_cels, cel);
            dwrite_insert_glyph_cel_to_table(glyph_table, glyph_index, cel);

            analysis->Release();
        }
        else  // glyph index exists in the inner-table
        {
            Glyph_Cel cel = entry->cel;
            dar_push(glyph_cels, cel);
        }
    }

    scratch_end(scratch);
}

internal Dwrite_Unit *
dwrite_alloc_unit(void)
{
    Dwrite_Unit *result = dwrite.first_free_unit;

    if (result != 0)
    {
        zero_struct(result);
        sll_pop_front(dwrite.first_free_unit, dwrite.last_free_unit);
    }
    else
    {
        result = push_struct(dwrite.unit_arena, Dwrite_Unit);
    }

    result->arena = arena_alloc();

    return result;
}

internal void
dwrite_release_unit(Dwrite_Unit *unit)
{
    sll_push_back(dwrite.first_free_unit, dwrite.last_free_unit, unit);
    arena_release(unit->arena);
}

internal Dwrite_Unit *
dwrite_map_text_to_glyphs(IDWriteFontFallback1 *font_fallback,
                          IDWriteFontCollection *font_collection,
                          IDWriteTextAnalyzer1 *text_analyzer,
                          WCHAR *locale, WCHAR *base_family,
                          FLOAT pt_per_em, FLOAT px_per_inch, WCHAR *text, u32 text_length)
{
    Temporary_Arena scratch = scratch_begin();

    Dwrite_Unit *unit = dwrite_alloc_unit();
    f32 max_advance_height_px = 0.0f; // @Todo: return this

    HRESULT hr = S_OK;

    u32 offset = 0;
    while (offset < text_length)
    {
        Dwrite_Font_Fallback_Result ff = dwrite_font_fallback(font_fallback, font_collection, base_family, locale,
                                                              text + offset, text_length - offset);
        u32 run_length = ff.length;
        IDWriteFontFace5 *run_font_face = ff.font_face;
        Assert(run_font_face);

        DWRITE_FONT_METRICS dfm = {};
        run_font_face->GetMetrics(&dfm);
        f32 du_per_em = dfm.designUnitsPerEm;
        f32 em_per_du = 1.0f / (f32)du_per_em;
        f32 px_per_pt = px_per_inch / 72.0f;
        f32 px_per_em = pt_per_em * px_per_pt;
        f32 px_per_du = px_per_em * em_per_du;

        f32 advance_height_px = (f32)(dfm.ascent + dfm.descent + dfm.lineGap) * px_per_du;
        max_advance_height_px = max(max_advance_height_px, advance_height_px);

        // --------------------------------------------------------------------
        // @Important: Must not free a font face for this to work.
        // @Note:      Update font table.
        Dwrite_Font_Metrics metrics = {};
        {
            metrics.du_per_em = du_per_em;
            metrics.advance_height_px = advance_height_px;
        }
        dwrite_insert_font_to_table(run_font_face, metrics);

        u16 *indices                 = 0;
        FLOAT *advances              = 0;
        DWRITE_GLYPH_OFFSET *offsets = 0;

        // Segment the run once again with identical complexity.
        WCHAR *remain_text = text + offset;
        u32 remain_length = run_length;
        while (remain_length)
        {
            Dwrite_Map_Complexity_Result complexity = dwrite_map_complexity(scratch.arena, text_analyzer, run_font_face, remain_text, remain_length);

            if (complexity.is_simple)
            {
                u32 glyph_count_add = complexity.mapped_length;
                u32 glyph_count_old = (u32)arrlenu(indices);
                u32 glyph_count_new = glyph_count_old + glyph_count_add;

                arrsetlen(unit->arena, indices,  glyph_count_new);
                arrsetlen(unit->arena, advances, glyph_count_new);
                arrsetlen(unit->arena, offsets,  glyph_count_new);

                s32 *advances_du = NULL;
                arrsetlen(unit->arena, advances_du, glyph_count_add);
                run_font_face->GetDesignGlyphAdvances(glyph_count_add, complexity.glyph_indices, advances_du, FALSE /*RetrieveVerticalAdvance*/);

                for (u32 i = 0; i < glyph_count_add; ++i)
                {
                    u32 idx = glyph_count_old + i;
                    indices[idx]  = complexity.glyph_indices[i];
                    advances[idx] = advances_du[i] * px_per_em * em_per_du; // @Todo: Unit?
                    offsets[idx]  = {};
                }
            }
            else // complex
            {
                u32 text_length = complexity.mapped_length;

                Dwrite_Text_Analysis_Sink analysis_sink = {};
                analysis_sink.arena = scratch.arena;

                Dwrite_Text_Analysis_Source analysis_source = {locale, remain_text, text_length};

                u16 *cluster_map                             = 0;
                DWRITE_SHAPING_TEXT_PROPERTIES *text_props   = 0;
                DWRITE_SHAPING_GLYPH_PROPERTIES *glyph_props = 0;

                u32 current_glyph_count = (u32)arrlenu(indices);
                u32 estimated_glyph_count_final = current_glyph_count + (3 * text_length) / 2 + 16;
                arrsetlen(unit->arena, indices, estimated_glyph_count_final);
                arrsetlen(unit->arena, advances, estimated_glyph_count_final);
                arrsetlen(unit->arena, offsets, estimated_glyph_count_final);

                // Split the text into runs of the same script ("language"), bidi, etc.
                hr = text_analyzer->AnalyzeScript(&analysis_source, 0/*textPosition*/, text_length, &analysis_sink);
                Assert(SUCCEEDED(hr));

                for (Dwrite_Text_Analysis_Sink_Result *analysis_sink_result = analysis_sink.result_first;
                     analysis_sink_result != 0;
                     analysis_sink_result = analysis_sink_result->next)
                {
                    u32 estimated_glyph_count_add = (3 * analysis_sink_result->text_length / 2 + 16);
                    u32 estimated_glyph_count_next = current_glyph_count + estimated_glyph_count_add;

                    if (arrlenu(cluster_map) < analysis_sink_result->text_length)
                    {
                        arrsetlen(unit->arena, cluster_map, analysis_sink_result->text_length);
                        arrsetlen(unit->arena, text_props, analysis_sink_result->text_length);
                    }

                    if (arrlenu(indices) < estimated_glyph_count_next)
                    {
                        arrsetlen(unit->arena, indices, estimated_glyph_count_next); 
                    }

                    if (arrlenu(glyph_props) < estimated_glyph_count_next)
                    {
                        arrsetlen(unit->arena, glyph_props, estimated_glyph_count_next); 
                    }

                    u32 actual_glyph_count_add = 0; 

                    u32 retry_count = 0;
                    while (retry_count < 8)
                    {
                        hr = text_analyzer->GetGlyphs(remain_text + analysis_sink_result->text_position,
                                                      analysis_sink_result->text_length,
                                                      run_font_face,
                                                      FALSE,                       // isSideways
                                                      0,                           // isRightToLeft,
                                                      &analysis_sink_result->analysis,
                                                      locale,
                                                      NULL,                        // numberSubstitution,
                                                      NULL,                        // features
                                                      NULL,                        // featureRangeLengths
                                                      0,                           // featureRanges
                                                      (u32)arrlenu(indices),

                                                      /* Out */
                                                      cluster_map,
                                                      text_props,
                                                      indices + current_glyph_count,
                                                      glyph_props,
                                                      &actual_glyph_count_add);

                        if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                        {
                            estimated_glyph_count_add *= 2;
                            estimated_glyph_count_next = current_glyph_count + estimated_glyph_count_add;
                            arrsetlen(unit->arena, indices, estimated_glyph_count_next);
                            arrsetlen(unit->arena, glyph_props, estimated_glyph_count_add);
                            retry_count++;
                        }
                        else if (FAILED(hr))
                        {
                            Assert(! "x");
                        }
                        else
                        {
                            break;
                        }
                    }

                    u32 actual_glyph_count_next = current_glyph_count + actual_glyph_count_add;
                    if (arrlenu(advances) < actual_glyph_count_next)
                    {
                        u64 size = (arrlenu(advances) << 1);
                        size = max(size, actual_glyph_count_add);
                        arrsetlen(unit->arena, advances, size);
                    }

                    hr = text_analyzer->GetGlyphPlacements(remain_text + analysis_sink_result->text_position,
                                                           cluster_map,
                                                           text_props,
                                                           analysis_sink_result->text_length,
                                                           indices + current_glyph_count,
                                                           glyph_props,
                                                           actual_glyph_count_add,
                                                           run_font_face,
                                                           px_per_em,
                                                           FALSE, // isSideways
                                                           0,     // isRightToLeft
                                                           &analysis_sink_result->analysis,
                                                           locale,
                                                           NULL,  // features
                                                           NULL,  // featureRangeLengths
                                                           0,     // featureRanges

                                                           /* out */
                                                           advances + current_glyph_count, // @Todo: Unit consistency.
                                                           offsets + current_glyph_count);

                    Assert(SUCCEEDED(hr));

                    current_glyph_count = actual_glyph_count_next;
                }

                arrsetlen(unit->arena, indices, current_glyph_count);
                arrsetlen(unit->arena, advances, current_glyph_count);
                arrsetlen(unit->arena, offsets, current_glyph_count);
            }

            remain_text += complexity.mapped_length;
            remain_length -= complexity.mapped_length;
        }

        Dwrite_Run *run = push_struct(unit->arena, Dwrite_Run);
        {
            run->e.fontFace      = run_font_face;
            run->e.fontEmSize    = px_per_em;
            run->e.glyphCount    = (u32)arrlenu(indices);
            run->e.glyphIndices  = indices;
            run->e.glyphAdvances = advances;
            run->e.glyphOffsets  = offsets;
        }
        sll_push_back(unit->run_first, unit->run_last, run);
        unit->run_count += 1;

        offset += run_length;
    }

    scratch_end(scratch);
    return unit;
}
