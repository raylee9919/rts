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
    for (u32 i = 0; i < RENDER_VERTEX_TYPE_COUNT; ++i)
    {
        Render_Buffer *buffer = renderer->buffer + i;
        buffer->vertices = push_array(renderer->arena, Render_Vertex, render_max_vertex_count);
    }

    // Note: Texture
    //
    renderer->texture_arena      = arena_alloc();
    renderer->texture_table_size = 2048;
    renderer->texture_table      = push_array(renderer->texture_arena, Render_Texture, renderer->texture_table_size);
    renderer->texture_next_id    = 1;

    // Note: Command buffer
    //
    renderer->command_arena = arena_alloc();
    renderer->commands = push_array(renderer->command_arena, Render_Command, render_max_command_count);
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
    // # Note: Clear buffer per frame.
    for (u32 i = 0; i < RENDER_VERTEX_TYPE_COUNT; ++i)
    {
        Render_Buffer *buffer = renderer->buffer + i;
        buffer->vertex_count   = 0;
        buffer->instance_count = 0;
    }

    // # Note: Clear command buffer.
    //
    renderer->command_count = 0;
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

internal void
render_command_push(Render_Command cmd)
{
    assert(renderer->command_count < render_max_command_count);
    renderer->commands[renderer->command_count++] = cmd;
}

internal Render_Texture *
render_texture_alloc(void)
{
    Render_Texture *texture = renderer->texture_free_first;

    if (texture != NULL)
    {
        sll_pop_front(renderer->texture_free_first, renderer->texture_free_last);
        zero_memory(texture, sizeof(Render_Texture));
    }
    else
    {
        texture = push_struct(renderer->texture_arena, Render_Texture);
    }

    return texture;
}

internal void
render_texture_release(Render_Texture *texture)
{
    sll_push_back(renderer->texture_free_first, renderer->texture_free_last, texture);
}

internal u64
render_key_from_id(Render_Id id)
{
    return id.e[0];
}

internal Render_Id
render_id_null(void)
{
    Render_Id id = {};
    return id;
}

internal b32
render_id_match(Render_Id a, Render_Id b)
{
    return (a.e[0] == b.e[0]);
}

internal Render_Texture *
render_texture_from_id(Render_Id id)
{
    Render_Texture *result = NULL;

    u64 slot = (render_key_from_id(id) % renderer->texture_table_size);

    for (Render_Texture *texture = renderer->texture_table[slot].first;
         texture != NULL;
         texture = texture->next)
    {
        if (render_id_match(texture->id, id))
        {
            result = texture;
            break;
        }
    }

    return result;
}

internal Render_Id
render_texture_create_flags(Render_Texture_Type type, void *data, u32 width, u32 height, Render_Command_Flags flags)
{
    Render_Id id;
    id.e[0] = renderer->texture_next_id++;

    // Get slot in table from id.
    u64 slot = render_key_from_id(id) % renderer->texture_table_size;

    // Alloc from free list.
    Render_Texture *texture = render_texture_alloc();
    {
        texture->id = id;
        texture->type = type;
        texture->data = data;
        texture->width = width;
        texture->height = height;
    }

    // Put into table.
    sll_push_back(renderer->texture_table[slot].first, renderer->texture_table[slot].last, texture);


    // Push command.
    Render_Command cmd = {};
    {
        cmd.flags = (RENDER_COMMAND_FLAG_TEXTURE_CREATE | flags);
        cmd.texture = *texture;
    }

    render_command_push(cmd);

    return id;
}

internal void
render_texture_destroy(Render_Id id)
{
    Render_Texture *texture = NULL;

    // Find texture from the table and remove the hash link.
    u64 slot = (render_key_from_id(id) % renderer->texture_table_size);
    Render_Texture *chain = renderer->texture_table + slot;
    for (Render_Texture *t = chain->first, *prev = NULL;
         t != NULL;
         prev = t, t = t->next)
    {

        if (render_id_match(t->id, id))
        {
            texture = t;

            if (t == chain->first)
            {
                chain->first = t->next;
            }

            if (t == chain->last)
            {
                chain->last = prev;
            }

            if (prev)
            {
                prev->next = t->next;
            }

            break;
        }
    }

    if (texture)
    {
        // Add to free list.
        render_texture_release(texture);

        // Push command.
        Render_Command cmd = {};
        {
            cmd.flags = RENDER_COMMAND_FLAG_TEXTURE_DESTROY;
            cmd.texture.id = id;
        }
        render_command_push(cmd);
    }
    else
    {
        assert(! "Texture not found in the table.");
    }
}


// # Note: Drawing Functions.
//
internal void
render_quad_tuv(Render_Id texture_id, v2 min, v2 max, v2 uv_min, v2 uv_max)
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
        uv_min, {uv_max.x,uv_min.y}, {uv_min.x,uv_max.y}, uv_max
    };
    
    for (u32 i = 0; i < 4; ++i)
    {
        // # alloc
        Render_Vertex *v = render_vertex_push(type);
        {
            // # init
            v->type        = type;
            v->position    = positions[i];
            v->uv          = uvs[i];
            v->texture_id  = texture_id;
        }
    }

    buffer->instance_count += 1;
}

// # Todo:
//
internal void
render_text(Utf8 text)
{
    u8 *ptr = text.str;
    u8 *opl = ptr + text.len;
    Unicode_Decode consume = {};
    for (;ptr < opl; ptr += consume.inc)
    {
        consume = utf8_decode(ptr, opl - ptr);
        u32 codepoint = consume.codepoint;
    }
}
