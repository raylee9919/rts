/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



internal Fp_State *
fp_alloc(void)
{
    Arena *arena = arena_alloc();
    Fp_State *result = push_struct(arena, Fp_State);
    result->arena = arena;

    return result;
}

internal void
fp_init(void)
{
    // @Todo: Make this modifiable to according dpi.
    fp_state->dpi = 96.0f;

    WCHAR *default_locale = L"en-US";
    if (! GetUserDefaultLocaleName(fp_state->locale, array_count(fp_state->locale)))
    {
        memory_copy(fp_state->locale, default_locale, sizeof(default_locale)); 
    }

    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(fp_state->factory), (IUnknown **)&fp_state->factory)))
    {
        assert(! "DWriteCreateFactory() Failed."); 
    }

    if (FAILED(fp_state->factory->CreateInMemoryFontFileLoader(&fp_state->in_memory_font_file_loader)))
    {
        assert(! "IDWriteFactory5::CreateInMemoryFontFileLoader() Failed."); 
    }

    if (FAILED(fp_state->factory->RegisterFontFileLoader(fp_state->in_memory_font_file_loader)))
    {
        assert(! "IDWriteFactory::RegisterFontFileLoader() Failed."); 
    }

    if (FAILED(fp_state->factory->GetSystemFontCollection(&fp_state->system_font_collection)))
    {
        assert(! "GetSystemFontCollection() Failed."); 
    }

    if (FAILED(fp_state->factory->GetSystemFontFallback(&fp_state->system_font_fallback)))
    {
        assert(! "GetSystemFontFallback() Failed."); 
    }

    if (FAILED(fp_state->system_font_fallback->QueryInterface(__uuidof(fp_state->system_font_fallback1), (void **)&fp_state->system_font_fallback1)))
    {
        assert(! "Error while querying IDWriteFontFallback1 interface."); 
    }

    if (FAILED(fp_state->factory->CreateTextAnalyzer(&fp_state->text_analyzer)))
    {
        assert(! "CreateTextAnalyzer() Failed."); 
    }

    if (FAILED(fp_state->text_analyzer->QueryInterface(__uuidof(fp_state->text_analyzer1), (void **)&fp_state->text_analyzer1)))
    {
        assert(! "Error while querying IDWriteTextAnalyzer1 interface."); 
    }

    if (FAILED(fp_state->factory->CreateRenderingParams(&fp_state->rendering_params)))
    {
        assert(! "IDWriteFactroy::CreateRenderingParams() Failed."); 
    }


    fp_state->font_table_size = 2048;
    fp_state->font_table = push_array(fp_state->arena, Fp_Font, fp_state->font_table_size);

    fp_state->run_arena = arena_alloc();
}

internal void
fp_add_font_from_memory(void *data, u64 size)
{
    IDWriteFontFile *font_file;
    if (FAILED(fp_state->in_memory_font_file_loader->CreateInMemoryFontFileReference(fp_state->factory, data, size, NULL, &font_file)))
    {
        assert(! "IDWriteInMemoryFontFileLoader::CreateInMemoryFontFileReference() Failed."); 
    }
}

internal Dwrite_Font_Fallback_Result
dwrite_font_fallback(IDWriteFontFallback1 *font_fallback, IDWriteFontCollection *font_collection, WCHAR *base_family,
                     WCHAR *text, u32 text_length)
{
    Dwrite_Font_Fallback_Result result = {};

    // @Note: It's safe to ignore scale in practice. -lhecker
    FLOAT dummy_scale;

    Dwrite_Text_Analysis_Source src = {fp_state->locale, text, text_length};
    font_fallback->MapCharacters(&src, 0/*offset*/, text_length, font_collection, base_family,
                                 NULL, 0,
                                 &result.length, &dummy_scale, &result.face);

    // @Todo: If no font contains the given codepoints MapCharacters() will return a NULL font_face.
    //        We need to replace them with ? glyphs, which this code doesn't do yet (by convention that's glyph index 0 in any font).
    assert(result.face);

    src.Release();
    return result;
}

internal Fp_Font *
dwrite_get_font_entry(IDWriteFontFace5 *face)
{
    Fp_Font *result = NULL;

    u64 slot = int_from_ptr(face) % fp_state->font_table_size;
    for (Fp_Font *entry = fp_state->font_table[slot].first; entry != NULL; entry = entry->next)
    {
        if (entry->face == face)
        {
            result = entry;
            break;
        }
    }

    return result;
}

internal Fp_Glyph *
dwrite_get_glyph(Fp_Font *font, u16 index)
{
    Fp_Glyph *result = NULL;

    u64 slot = index % font->glyph_table_size;
    for (Fp_Glyph *entry = font->glyph_table[slot].first; entry != NULL; entry = entry->next)
    {
        if (entry->index == index)
        {
            result = entry;
            break;
        }
    }

    return result;
}

internal Fp_Font *
dwrite_font_alloc(IDWriteFontFace5 *face)
{
    u64 slot = int_from_ptr(face) % fp_state->font_table_size;

    Fp_Font *font = push_struct(fp_state->arena, Fp_Font);
    sll_push_back(fp_state->font_table[slot].first, fp_state->font_table[slot].last, font);

    return font;
}


// @Note: Determines the longest run of characters that map 1:1 to glyphs without
//        ambiguity. In that case, it returns TRUE and you can immediately use indices.
//        Otherwise, perform full glyph shaping.
internal Dwrite_Map_Complexity_Result
dwrite_map_complexity(Arena *arena, IDWriteFontFace *face,
                      WCHAR *text, u32 text_length)
{
    Dwrite_Map_Complexity_Result result = {};

    b32 is_simple;
    u32 mapped_length;
    u32 index_cap = text_length;
    u16 *_indices = push_array(arena, u16, index_cap);

    HRESULT hr = fp_state->text_analyzer1->GetTextComplexity(text, text_length, face,
                                                             /* out */
                                                             &is_simple, &mapped_length, _indices);
    assert(SUCCEEDED(hr));

    result.glyph_indices = _indices;
    result.is_simple     = is_simple;
    result.mapped_length = mapped_length;

    return result;
}

internal Fp_Run *
dwrite_runs_from_string(Utf8 string, Utf8 base_family8, f32 font_size)
{
    ProfileScope;

    Temporary_Arena scratch = scratch_begin();
    HRESULT hr = S_OK;

    Fp_Run *run_first = NULL;
    Fp_Run *run_last  = NULL;

    Utf16 base_family16 = to_utf16(scratch.arena, base_family8);
    WCHAR *base_family = (WCHAR *)base_family16.str;
    WCHAR *locale = fp_state->locale;
    Utf16  string16 = to_utf16(scratch.arena, string);

    u32 offset = 0;
    while (offset < string16.len)
    {
        Dwrite_Font_Fallback_Result ff = dwrite_font_fallback(fp_state->system_font_fallback1, fp_state->system_font_collection, base_family,
                                                              (WCHAR *)string16.str + offset, string16.len - offset);
        u32 run_length = ff.length;
        IDWriteFontFace5 *run_face = ff.face;
        assert(run_face);

        DWRITE_FONT_METRICS dfm = {};
        run_face->GetMetrics(&dfm);
        f32 du_per_em = dfm.designUnitsPerEm;
        f32 em_per_du = 1.f / (f32)du_per_em;
        f32 px_per_pt = fp_state->dpi / 72.f;
        f32 px_per_em = font_size * px_per_pt;
        f32 px_per_du = px_per_em * em_per_du;

        // @Important: Must not free a font face for this to work.
        Fp_Font *font = dwrite_get_font_entry(run_face);
        if (font == NULL)
        {
            font = dwrite_font_alloc(run_face);
            font->face              = run_face;

            font->font_size         = font_size;
            font->ascent            = dfm.ascent  * px_per_du;
            font->descent           = dfm.descent * px_per_du;
            font->linegap           = dfm.lineGap * px_per_du;
            font->arena             = arena_alloc();
            font->glyph_table_size  = 1024;
            font->glyph_table       = push_array(font->arena, Fp_Glyph, font->glyph_table_size);

            font->atlas.width  = 1024;
            font->atlas.height = 1024;
            font->atlas.pitch  = 1024<<2;
            font->atlas.data   = push_array(font->arena, u8, font->atlas.pitch * font->atlas.height);

            font->atlas.rpk_ctx = push_struct(font->arena, Rpk_Context);
            rpk_init(font->atlas.rpk_ctx, font->arena, font->atlas.width, font->atlas.height);

            font->atlas.id = render_texture_create_filter_dot(RENDER_TEXTURE_TYPE_R8G8B8A8, font->atlas.data, font->atlas.width, font->atlas.height);
        }

        u16 *indices                 = NULL;
        FLOAT *advances              = NULL;
        DWRITE_GLYPH_OFFSET *offsets = NULL;

        // Segment the run once again with identical complexity.
        WCHAR *remain_text = (WCHAR *)string16.str + offset;
        u32 remain_length = run_length;
        while (remain_length)
        {
            Dwrite_Map_Complexity_Result complexity = dwrite_map_complexity(scratch.arena, run_face, remain_text, remain_length);

            if (complexity.is_simple)
            {
                u32 glyph_count_add = complexity.mapped_length;
                u32 glyph_count_old = (u32)arrlenu(indices);
                u32 glyph_count_new = glyph_count_old + glyph_count_add;

                arrsetlen(fp_state->run_arena, indices,  glyph_count_new);
                arrsetlen(fp_state->run_arena, advances, glyph_count_new);
                arrsetlen(fp_state->run_arena, offsets,  glyph_count_new);

                s32 *advances_du = NULL;
                arrsetlen(fp_state->run_arena, advances_du, glyph_count_add);
                run_face->GetDesignGlyphAdvances(glyph_count_add, complexity.glyph_indices, advances_du, FALSE /*RetrieveVerticalAdvance*/);

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
                arrsetlen(fp_state->run_arena, indices, estimated_glyph_count_final);
                arrsetlen(fp_state->run_arena, advances, estimated_glyph_count_final);
                arrsetlen(fp_state->run_arena, offsets, estimated_glyph_count_final);

                // Split the text into runs of the same script ("language"), bidi, etc.
                hr = fp_state->text_analyzer1->AnalyzeScript(&analysis_source, 0/*textPosition*/, text_length, &analysis_sink);
                assert(SUCCEEDED(hr));

                for (Dwrite_Text_Analysis_Sink_Result *analysis_sink_result = analysis_sink.result_first;
                     analysis_sink_result != 0;
                     analysis_sink_result = analysis_sink_result->next)
                {
                    u32 estimated_glyph_count_add = (3 * analysis_sink_result->text_length / 2 + 16);
                    u32 estimated_glyph_count_next = current_glyph_count + estimated_glyph_count_add;

                    if (arrlenu(cluster_map) < analysis_sink_result->text_length)
                    {
                        arrsetlen(fp_state->run_arena, cluster_map, analysis_sink_result->text_length);
                        arrsetlen(fp_state->run_arena, text_props, analysis_sink_result->text_length);
                    }

                    if (arrlenu(indices) < estimated_glyph_count_next)
                    {
                        arrsetlen(fp_state->run_arena, indices, estimated_glyph_count_next); 
                    }

                    if (arrlenu(glyph_props) < estimated_glyph_count_next)
                    {
                        arrsetlen(fp_state->run_arena, glyph_props, estimated_glyph_count_next); 
                    }

                    u32 actual_glyph_count_add = 0; 

                    u32 retry_count = 0;
                    while (retry_count < 8)
                    {
                        hr = fp_state->text_analyzer1->GetGlyphs(remain_text + analysis_sink_result->text_position,
                                                                 analysis_sink_result->text_length,
                                                                 run_face,
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
                            arrsetlen(fp_state->run_arena, indices, estimated_glyph_count_next);
                            arrsetlen(fp_state->run_arena, glyph_props, estimated_glyph_count_add);
                            retry_count++;
                        }
                        else if (FAILED(hr))
                        {
                            assert(! "x");
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
                        arrsetlen(fp_state->run_arena, advances, size);
                    }

                    hr = fp_state->text_analyzer1->GetGlyphPlacements(remain_text + analysis_sink_result->text_position,
                                                                      cluster_map,
                                                                      text_props,
                                                                      analysis_sink_result->text_length,
                                                                      indices + current_glyph_count,
                                                                      glyph_props,
                                                                      actual_glyph_count_add,
                                                                      run_face,
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

                    assert(SUCCEEDED(hr));

                    current_glyph_count = actual_glyph_count_next;
                }

                arrsetlen(fp_state->run_arena, indices, current_glyph_count);
                arrsetlen(fp_state->run_arena, advances, current_glyph_count);
                arrsetlen(fp_state->run_arena, offsets, current_glyph_count);


                analysis_sink.Release();
                analysis_source.Release();
            }

            remain_text += complexity.mapped_length;
            remain_length -= complexity.mapped_length;
        }

        Fp_Run *run = push_struct(fp_state->run_arena, Fp_Run);
        {
            run->face         = run_face;
            run->font_size    = font_size;
            run->count        = (u32)arrlenu(indices);
            run->indices      = indices;
            run->advances     = advances;
        }
        sll_push_back(run_first, run_last, run);

        offset += run_length;
    }

    scratch_end(scratch);
    return run_first;
}

internal void
fp_pack_run(Fp_Run *run, b32 is_cleartype)
{
    ProfileScope;

    HRESULT hr = S_OK;
    Temporary_Arena scratch = scratch_begin();

    IDWriteFontFace5 *face = run->face;
    f32 font_size = run->font_size;
    u64 glyph_count = run->count;

    DWRITE_FONT_METRICS dfm = {};
    face->GetMetrics(&dfm);
    f32 du_per_em = dfm.designUnitsPerEm;
    f32 px_per_pt = fp_state->dpi / 72.f;
    f32 px_per_em = font_size * px_per_pt;
    f32 px_per_du = px_per_em * (1.f/du_per_em);

    // @Temporary:
    b32 is_sideways = 0;
    int bidi_level  = 0;


    // @Note: Create rendering mode of a font face.
    //
    DWRITE_RENDERING_MODE1 rendering_mode = DWRITE_RENDERING_MODE1_NATURAL;
    DWRITE_MEASURING_MODE measuring_mode  = DWRITE_MEASURING_MODE_NATURAL;
    DWRITE_GRID_FIT_MODE grid_fit_mode    = DWRITE_GRID_FIT_MODE_DEFAULT;

    assert(SUCCEEDED(face->GetRecommendedRenderingMode(px_per_em,
                                                       fp_state->dpi, fp_state->dpi,
                                                       NULL, // transform
                                                       is_sideways,
                                                       DWRITE_OUTLINE_THRESHOLD_ANTIALIASED,
                                                       measuring_mode,
                                                       fp_state->rendering_params,
                                                       &rendering_mode,
                                                       &grid_fit_mode)));


    Fp_Font *font_entry = dwrite_get_font_entry(face);
    assert(font_entry);

    // @Todo: Think about floating point mathematics.
    if (absolute(font_entry->font_size - font_size) > 0.1f)
    {
        ProfileScopeN("RebuildAtlas");

        Fp_Font *fe = font_entry;

        fe->font_size = font_size;
        fe->ascent    = dfm.ascent  * px_per_du;
        fe->descent   = dfm.descent * px_per_du;
        fe->linegap   = dfm.lineGap * px_per_du;

        arena_clear(fe->arena);

        fe->atlas.width  = 1024;
        fe->atlas.height = 1024;
        fe->atlas.pitch  = 1024<<2;
        fe->atlas.data   = push_array(fe->arena, u8, fe->atlas.pitch * fe->atlas.height);

        fe->atlas.rpk_ctx = push_struct(fe->arena, Rpk_Context);
        rpk_init(fe->atlas.rpk_ctx, fe->arena, fe->atlas.width, fe->atlas.height);

        fe->glyph_table = push_array(fe->arena, Fp_Glyph, fe->glyph_table_size);
    }

    DWRITE_TEXTURE_TYPE texture_type = (is_cleartype) ? DWRITE_TEXTURE_CLEARTYPE_3x1 : DWRITE_TEXTURE_ALIASED_1x1;

    // Check if each glyph in the run exists in the inner hash table.
    for (u32 i = 0; i < glyph_count; ++i)
    {
        u16 glyph_index = run->indices[i];

        Fp_Glyph *entry = dwrite_get_glyph(font_entry, glyph_index);

        if (! entry) // glyph index doesn't exist in the inner-table
        {
            // Get single glyph's metrics.
            DWRITE_GLYPH_METRICS metrics = {};
            assert(SUCCEEDED(face->GetDesignGlyphMetrics(&glyph_index, 1, &metrics, is_sideways)));

            // CreateGlyphRunAnalysis() doesn't support DWRITE_RENDERING_MODE_OUTLINE.
            // We won't bother big glyphs. (many hundreds of pt)
            if (rendering_mode == DWRITE_RENDERING_MODE1_OUTLINE)
            {
                rendering_mode = DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC; 
            }

            DWRITE_GLYPH_RUN single_glyph_run = {};
            {
                single_glyph_run.fontFace      = face;
                single_glyph_run.fontEmSize    = px_per_em;
                single_glyph_run.glyphCount    = 1;
                single_glyph_run.glyphIndices  = &glyph_index;
                single_glyph_run.glyphAdvances = NULL;
                single_glyph_run.glyphOffsets  = NULL;
                single_glyph_run.isSideways    = is_sideways;
                single_glyph_run.bidiLevel     = bidi_level;
            }

            IDWriteGlyphRunAnalysis *analysis = NULL;
            assert(SUCCEEDED(fp_state->factory->CreateGlyphRunAnalysis(&single_glyph_run,
                                                                           NULL, // transform
                                                                           rendering_mode,
                                                                           measuring_mode,
                                                                           grid_fit_mode,
                                                                           is_cleartype ? DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE : DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE,
                                                                           0.0f, // baselineOriginX
                                                                           0.0f, // baselineOriginY
                                                                           &analysis)));

            // @Note: GetAlphaTextureBounds()
            //
            //         bounds.top ------++-----######--+
            //           (-7)           ||  ############
            //                          ||###@     ####
            //                          |##@      #####
            //          baseline ______ |##@     #####|
            //           origin        \|############@|
            //          (= 0,0)         \|##########@ |
            //                          ++-------###---+
            //                          #@     ##@   |
            //         bounds.bottom ---+#########-----+
            //            (+2)          |              |
            //                     bounds.left     bounds.right
            //                         (-1)           (+14)
            //        

            RECT bounds = {};
            hr = analysis->GetAlphaTextureBounds(texture_type, &bounds);
            if (FAILED(hr))
            {
                // @Todo: The font doesn't support DWRITE_TEXTURE_CLEARTYPE_3x1.
                //         Retry with DWRITE_TEXTURE_ALIASED_1x1.
                assert(! "x");
            }

            Fp_Glyph *glyph = push_struct(font_entry->arena, Fp_Glyph);

            Fp_Atlas *atlas = &font_entry->atlas;

            if ((bounds.right > bounds.left) && (bounds.bottom > bounds.top))
            {
                u32 blackbox_width  = bounds.right - bounds.left;
                u32 blackbox_height = bounds.bottom - bounds.top;

                u32 margin = 1;
                u32 bitmap_width  = blackbox_width + 2*margin;
                u32 bitmap_height = blackbox_height + 2*margin;

                u32 rgb_bitmap_size = (is_cleartype) ? (blackbox_width*3)*blackbox_height : blackbox_width*blackbox_height; 
                u8 *bitmap_data_rgb = (u8 *)push_size(scratch.arena, rgb_bitmap_size);
          
                // @Note: Profiled. CreateAlphaTexture() takes about 7-cycles including assertion.
                assert(SUCCEEDED(analysis->CreateAlphaTexture(texture_type, &bounds, bitmap_data_rgb, rgb_bitmap_size)));

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
                            dst[0] = src[0];
                            dst[1] = src[1];
                            dst[2] = src[2];
                            // @Note: Alpha doesn't matter since Cleatype doesn't handle alpha.
                            //         But I guess setting alpha to 1 is better...?
                            //
                            u8 *texel = dst;
                            if (texel[0] != 0 || texel[1] != 0 || texel[2] != 0)
                            {
                                texel[3] = 0xff;
                            }
                        }
                    }
                }
                else
                {
                    assert(! "Couldn't fit in the atlas");
                }

                glyph->index    = glyph_index;
                glyph->uv_min   = {(f32)(x1 + margin) / (f32)atlas->width, (f32)(y1 + margin) / (f32)atlas->height};
                glyph->uv_max   = {(f32)(x2 - margin) / (f32)atlas->width, (f32)(y2 - margin) / (f32)atlas->height};
                glyph->lsb      = bounds.left;
                glyph->rsb      = bounds.right;
                glyph->tsb      = bounds.top;
                glyph->bsb      = bounds.bottom;
                
                font_entry->atlas.dirty = 1;
            }
            else
            {
                glyph->index  = glyph_index;
                glyph->uv_min = v2{0.f, 0.f};
                glyph->uv_max = v2{0.f, 0.f};
                glyph->lsb    = 0.f;
                glyph->rsb    = 0.f;
                glyph->tsb    = 0.f;
                glyph->bsb    = 0.f;
            }

            u64 slot = glyph_index % font_entry->glyph_table_size;
            sll_push_back(font_entry->glyph_table[slot].first, font_entry->glyph_table[slot].last, glyph);

            analysis->Release();
        }
    }

    if (font_entry->atlas.dirty)
    {
        font_entry->atlas.dirty = 0;

        render_texture_update(font_entry->atlas.id, font_entry->atlas.data);
    }

    scratch_end(scratch);
}

internal Fp_Draw_String_Result
fp_draw_string(Utf8 string, Utf8 base_family, f32 font_size, v2 origin, Render_String_Flags flags, AABB2 cull_aabb)
{
    ProfileScope;

    Fp_Run *runs = dwrite_runs_from_string(string, base_family, font_size);

    for (Fp_Run *run = runs; run != NULL; run = run->next)
    {
        b32 is_cleartype = 1;
        fp_pack_run(run, is_cleartype);
    }

    Fp_Draw_String_Result result = {};
    AABB2 aabb      = {v2{ F32_MAX,  F32_MAX}, v2{-F32_MAX, -F32_MAX}};
    f32 max_ascent  = -F32_MAX;
    f32 max_descent = -F32_MAX;

    v2 pen = origin;
    for (Fp_Run *run = runs; run != NULL; run = run->next)
    {
        IDWriteFontFace5 *face = run->face;

        Fp_Font *font = dwrite_get_font_entry(face);
        assert(font);
        
        f32 ascent  = font->ascent;
        f32 descent = font->descent;

        max_ascent  = max(max_ascent, ascent);
        max_descent = max(max_descent, descent);

        Render_Id atlas = font->atlas.id;

        for (u32 i = 0; i < run->count; ++i)
        {
            u16 glyph_index = run->indices[i];

            Fp_Glyph *glyph = dwrite_get_glyph(font, glyph_index);
            assert(glyph);

            v2 min = pen + v2{glyph->lsb, glyph->tsb};
            v2 max = pen + v2{glyph->rsb, glyph->bsb};

            min.x = floorf(min.x);
            min.y = floorf(min.y);
            max.x = floorf(max.x);
            max.y = floorf(max.y);

            if (! (flags & RENDER_STRING_FLAG_NO_DRAW))
            {
                f32 w = glyph->rsb - glyph->lsb;
                f32 h = glyph->bsb - glyph->tsb;

                v2 uv_min = glyph->uv_min;
                v2 uv_max = glyph->uv_max;

                if (flags & RENDER_STRING_FLAG_CULL)
                {
                    AABB2 cel = {};
                    cel.min = min;
                    cel.max = max;

                    if (intersects(cull_aabb, cel))
                    {
                        // @Todo: intersection() does some duplicate operations to intersects().
                        AABB2 overlap = intersection(cull_aabb, cel);

                        v2 box_range_x = v2{min.x, max.x};
                        v2 box_range_y = v2{min.y, max.y};

                        uv_min.x = lerp(uv_min.x,  normalize01(box_range_x, overlap.min.x), uv_max.x);
                        uv_max.x = lerp(uv_min.x,  normalize01(box_range_x, overlap.max.x), uv_max.x);
                        uv_min.y = lerp(uv_min.y,  normalize01(box_range_y, overlap.min.y), uv_max.y);
                        uv_max.y = lerp(uv_min.y,  normalize01(box_range_y, overlap.max.y), uv_max.y);

                        render_quad_tuv(atlas, overlap.min, overlap.max, uv_min, uv_max);
                    }
                }
                else
                {
                    render_quad_tuv(atlas, min, max, uv_min, uv_max);
                }
            }

            // # Note: Update string aabb.
            if (flags & RENDER_STRING_FLAG_COMPUTE_SIZE)
            {
                aabb.min.x = min(aabb.min.x, min.x);
                aabb.min.y = min(aabb.min.y, min.y);
                aabb.max.x = max(aabb.max.x, max.x);
                aabb.max.y = max(aabb.max.y, max.y);
            }

            f32 advance = run->advances[i];
            pen.x += advance;
        }
    }


    arena_clear(fp_state->run_arena);

    result.aabb        = aabb;
    result.max_ascent  = max_ascent;
    result.max_descent = max_descent;
    return result;
}
