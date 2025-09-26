/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#define push_render_entity(GROUP, STRUCT)  (STRUCT *)__push_render_entity(GROUP, sizeof(STRUCT), e##STRUCT)
internal Render_Entity_Header *
__push_render_entity(Render_Group *renderGroup, u32 size, Render_Type type)
{
    assert(size + renderGroup->used <= renderGroup->capacity);

    Render_Entity_Header *header = (Render_Entity_Header *)(renderGroup->base + renderGroup->used);
    header->type = type;
    header->size = size;

    renderGroup->used += size;

    return header;
}

internal void
push_mesh(Render_Group *group, Mesh *mesh,
          m4x4 world_transform, m4x4 *animation_transforms, u32 entity_id, v2 uv_scale, v4 tint)
{
    Render_Mesh *piece          = push_render_entity(group, Render_Mesh);
    piece->mesh                 = mesh;
    piece->world_transform      = world_transform;
    piece->animation_transforms = animation_transforms;
    piece->entity_id            = entity_id;
    piece->uv_scale             = uv_scale;
    piece->tint                 = tint;
}

internal void
push_mesh(Render_Group *group, Mesh *mesh,
          m4x4 world_transform, m4x4 *animation_transforms, u32 entity_id, v2 uv_scale)
{
    push_mesh(group, mesh, world_transform, animation_transforms, entity_id, uv_scale, V4(1.0f));
}

internal void
draw_triangles(Render_Group *group, Vertex *vertices, u32 vertexcount, u32 *indices, u32 numtri, v4 color)
{
    Render_Triangles *piece = push_render_entity(group, Render_Triangles);
    piece->vertices = vertices;
    piece->vertexcount = vertexcount;
    piece->indices = indices;
    piece->numtri = numtri;
    piece->color = color;
}

internal void
draw_line(Render_Group *group, v3 a, v3 b, v4 color) 
{
    Render_Line *piece = push_render_entity(group, Render_Line);
    piece->p[0] = a;
    piece->p[1] = b;
    piece->color = color;
}

internal void
push_bitmap(Render_Group *group,
            v3 min, v3 max,
            Bitmap *bitmap = 0, v4 color = V4(1, 1, 1, 1))
{
    Render_Bitmap *piece = push_render_entity(group, Render_Bitmap);
    piece->min    = min;
    piece->max    = max;
    piece->bitmap = bitmap;
    piece->color  = color;
}

typedef u8 String_Op;
enum
{
    String_Op_Draw     = (1<<0), 
    String_Op_Get_Rect = (1<<1),
};

internal Rect2
string_op(u8 flag, Render_Group *render_group,
          v3 left_bottom,
          char *str, Asset_Font *font, v4 color = v4{1, 1, 1, 1})
{
    Rect2 result = rect2_inv_inf();

    f32 cur_x = left_bottom.x;
    f32 cur_y = left_bottom.y;
    f32 kern  = 0.0f;
    f32 A     = 0.0f;

    for (const char *ch = str; *ch; ++ch) 
    {
        switch (*ch) 
        {
            case ' ': {
                Asset_Glyph *glyph = font->glyphs[*ch];
                assert(glyph);
                f32 B = (f32)glyph->B;
                f32 C = (f32)glyph->C;

                f32 max_x = cur_x + B + C;
                f32 min_x = cur_x;
                cur_x += (B + C);

                if (flag & String_Op_Get_Rect) 
                {
                    result.min.x = min(result.min.x, min_x);
                    result.max.x = max(result.max.x, max_x);
                }
            } break;

            case '\n': {
                cur_x = left_bottom.x;
                cur_y -= font->v_advance;
                result.min.y = cur_y;
            } break;

            default: {
                Asset_Glyph *glyph = font->glyphs[*ch];
                assert(glyph);
                Bitmap *bitmap = &glyph->bitmap;
                f32 bw = (f32)bitmap->width;
                f32 bh = (f32)bitmap->height;
                v3 max = v3{cur_x + bw, cur_y + glyph->ascent, left_bottom.z};
                v3 min = max - v3{bw, bh, 0};

                if (flag & String_Op_Draw)
                { push_bitmap(render_group, min, max, bitmap, color); }

                if (flag & String_Op_Get_Rect) 
                {
                    result.min.x = min(result.min.x, min.x);
                    result.min.y = min(result.min.y, min.y);
                    result.max.x = max(result.max.x, max.x);
                    result.max.y = max(result.max.y, max.y);
                }

                cur_x += bw;
            } break;
        }

        if (*(ch + 1)) 
        {
            kern = (f32)get_kerning(&font->kern_hashmap, *ch, *(ch + 1));
            if (font->glyphs[*(ch + 1)]) 
            { A = (f32)font->glyphs[*(ch + 1)]->A; }
            f32 advance_x = (A + kern);
            cur_x += advance_x;
        }
    }

    if (result.min.x == F32_MAX) 
    {
        result = {}; 
    }

    return result;
}

internal Render_Group *
begin_render_group(Render_Commands *frame, u64 size) 
{
    assert(frame->push_buffer_used + sizeof(Render_Group) + size <= frame->push_buffer_size);

    Render_Group *group = (Render_Group *)(frame->push_buffer_base + frame->push_buffer_used);
    frame->push_buffer_used += sizeof(Render_Group);
    group->capacity = size;
    group->base     = (frame->push_buffer_base + frame->push_buffer_used);
    group->used     = 0;
    frame->push_buffer_used += size;

    return group;
}


// # Todo: Revamping renderer currently..
//
internal Render_Vertex *
render_vertex_push(Render_Vertex_Type type)
{
    Render_Vertex *result = NULL;

    Render_Buffer *buffer = renderer->buffer + type;

    if (buffer->vertex_count < render_max_vertex_count)
    {
        result = buffer->vertices + buffer->vertex_count;
        buffer->vertex_count += 1;
    }
    else
    {
        assert("! exceeded maximum # of render entity.");
    }

    return result;
}

// # Note: Init Function
//
internal void
render_init(void)
{
    renderer->initted = true;

    for (u32 i = 0; i < RENDER_VERTEX_TYPE_COUNT; ++i)
    {
        Render_Buffer *buffer = renderer->buffer + i;
        buffer->vertices = push_array(renderer->arena, Render_Vertex, render_max_vertex_count);
    }
}

// # Note: Sort Cmp Functions
//
internal int
render_cmp_texture_id(const void *a, const void *b)
{
    Render_Vertex *p = (Render_Vertex *)a;
    Render_Vertex *q = (Render_Vertex *)b;
    
    // # Todo:
    return false;
}

// # Note: Render Begin/End Pair
//
internal void
render_begin(void)
{
    // # Clear buffer per frame.
    for (u32 i = 0; i < RENDER_VERTEX_TYPE_COUNT; ++i)
    {
        Render_Buffer *buffer = renderer->buffer + i;
        buffer->vertex_count   = 0;
        buffer->instance_count = 0;
    }
}

internal void
render_end(void)
{
    for (u32 type = 0; type < RENDER_VERTEX_TYPE_COUNT; ++type)
    {
        Render_Buffer *buffer = renderer->buffer + type;
        u64 count = buffer->vertex_count;

        switch (type)
        {
            case RENDER_VERTEX_TYPE_QUAD: 
            {
                //quick_sort(buffer->vertices, Render_Vertex, count, render_cmp_texture_id);
            } break;

            default: 
            {
                assert(! "not implemented yet?");
            } break;
        }
    }
}


// # Note: Drawing Functions.
//
internal void
draw_quad(v2 min, v2 max)
{
    // # Note: Order and UV
    //
    //         [0,0]  [1,0]
    //            0----1
    //            |    |
    //            2----3
    //         [0,1]  [1,1]
    //
    Render_Vertex_Type type = RENDER_VERTEX_TYPE_QUAD;
    Render_Buffer *buffer = renderer->buffer + type;

    v2 positions[4] = {
        min, {max.x, min.y}, {min.x, max.y}, max 
    };

    v2 uvs[4] = {
        {0,0}, {1,0}, {0,1}, {1,1}
    };
    
    for (u32 i = 0; i < 4; ++i)
    {
        // # alloc
        Render_Vertex *v = render_vertex_push(type);
        {
            // # init
            v->type     = type;
            v->position = positions[i];
            v->uv       = uvs[i];
        }
    }

    buffer->instance_count += 1;
}
