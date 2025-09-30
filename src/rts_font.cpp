/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// # Note: Face alloc
//
internal Face *
face_alloc(void)
{
    Arena *arena = arena_alloc();
    Face *face = push_struct(arena, Face);
    {
        face->arena = arena;

        // # Alloc metrics table.
        {
            u64 table_size = 1024;
            face->metrics_table = push_array(arena, Glyph_Metrics, table_size);
            face->metrics_table_size = table_size;
        }

        // # Alloc CMAP
        {
            u64 table_size = 1024;
            face->cmap_table = push_array(arena, Face_Cmap, table_size);
            face->cmap_table_size = table_size;
        }
    }

    return face;
}


// # Note: Glyph metrics.
//
internal u64
glyph_metrics_slot_from_index(Face *face, u16 glyph_index)
{
    // # Todo: Better hash?
    return glyph_index % face->metrics_table_size;
}

internal Glyph_Metrics *
glyph_metrics_get(Face *face, u16 glyph_index)
{
    Glyph_Metrics *result = NULL;
    u64 slot = glyph_metrics_slot_from_index(face, glyph_index);

    for (Glyph_Metrics *metrics = face->metrics_table[slot].first;
         metrics != NULL;
         metrics = metrics->next)
    {
        if (metrics->glyph_index == glyph_index)
        {
            result = metrics;
            break;
        }
    }

    return result;
}

internal void
glyph_metrics_put(Face *face, Glyph_Metrics metrics)
{
    u64 slot = glyph_metrics_slot_from_index(face, metrics.glyph_index);
    Glyph_Metrics *list = face->metrics_table + slot;

    for (Glyph_Metrics *entry = list->first;
         entry != NULL;
         entry = entry->next)
    {
        if (entry->glyph_index == metrics.glyph_index)
        {
            *entry = metrics;
            return;
        }
    }


    Glyph_Metrics *item = push_struct(face->arena, Glyph_Metrics);
    *item = metrics;
    sll_push_back(list->first, list->last, item);
}





// # Note: CMAP
//
internal u64
face_cmap_slot_from_codepoint(Face *face, u32 codepoint)
{
    // # Todo: Better hash?
    return (codepoint % face->cmap_table_size);
}

internal Face_Cmap *
face_cmap_get(Face *face, u32 codepoint)
{
    Face_Cmap *result = NULL;
    u64 slot = face_cmap_slot_from_codepoint(face, codepoint);

    for (Face_Cmap *map = face->cmap_table[slot].first;
         map != NULL;
         map = map->next)
    {
        if (map->codepoint == codepoint)
        {
            result = map;
            break;
        }
    }

    return result;
}

internal void
face_cmap_put(Face *face, u32 codepoint, u16 *indices, u64 index_count)
{
    u64 index_total_size = sizeof(*indices) * index_count;
    u64 slot = face_cmap_slot_from_codepoint(face, codepoint);

    Face_Cmap *found = NULL;
    Face_Cmap *list = face->cmap_table + slot;
    for (Face_Cmap *map = list->first;
         map != NULL;
         map = map->next)
    {
        if (map->codepoint == codepoint)
        {
            found = map;
            break;
        }
    }

    if (! found)
    {
        found = push_struct(face->arena, Face_Cmap);
        found->codepoint = codepoint;
        sll_push_back(list->first, list->last, found);
    }

    found->indices = push_array(face->arena, u16, index_count);
    found->index_count = index_count;
    memory_copy(found->indices, indices, index_total_size);
}

internal u64
face_cmap_glyph_count_from_codepoint(Face *face, u32 codepoint)
{
    u64 count = 0;
    Face_Cmap *map = face_cmap_get(face, codepoint);
    if (map)
    {
        count = map->index_count;
    }
    return count;
}

internal u16 *
face_cmap_glyph_indices_from_codepoint(Face *face, u32 codepoint)
{
    u16 *result = NULL;
    Face_Cmap *map = face_cmap_get(face, codepoint);
    if (map)
    {
        result = map->indices;
    }
    return result;
};
