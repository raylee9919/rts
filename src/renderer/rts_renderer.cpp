/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


#include "geometry_generator.cpp"


#define push_render_entity(GROUP, STRUCT)  (STRUCT *)__push_render_entity(GROUP, sizeof(STRUCT), e##STRUCT)
internal Render_Entity_Header *
__push_render_entity(Render_Group *renderGroup, u32 size, Render_Type type)
{
    Assert(size + renderGroup->used <= renderGroup->capacity);

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
draw_line(Render_Group *group, v3 a, v3 b, v4 color) {
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

internal void
push_rect(Render_Group *group, Rect2 rect, f32 z, v4 color)
{
    push_bitmap(group, V3(rect.min, z), V3(rect.max, z), 0, color);
}

internal void
push_bordered_rect(Render_Group *group, Rect2 rect, f32 z, v4 color, f32 borderlength, v4 bordercolor = RGBA_BLACK)
{
    Rect2 cenrect = add_radius_to(rect, V2(-borderlength));
    v2 min = cenrect.min;
    v2 max = cenrect.max;

    push_rect(group, cenrect, z, color);

    Rect2 r[4];
    f32 b = borderlength;
    r[0] = Rect2{v2{min.x - b, min.y - b}, v2{max.x, min.y}};
    r[1] = Rect2{v2{max.x, min.y - b}, v2{max.x + b, max.y}};
    r[2] = Rect2{v2{min.x, max.y}, v2{max.x + b, max.y + b}};
    r[3] = Rect2{v2{min.x - b, min.y}, v2{min.x, max.y + b}};
    for (int i = 0; i < 4; ++i) {
        push_rect(group, r[i], z, bordercolor);
    }
}

enum String_Op : u8 {
    String_Op_Draw     = 0x1,
    String_Op_Get_Rect = 0x2,
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

    for (const char *ch = str; *ch; ++ch) {
        switch (*ch) {
            case ' ': {
                Asset_Glyph *glyph = font->glyphs[*ch];
                Assert(glyph);
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
                Assert(glyph);
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
    { result = {}; }

    return result;
}

internal void
push_shadowed_string(Render_Group *group, v3 leftbottom, char *str, Asset_Font *font, v4 color = v4{1,1,1,1}) {
    const v4 shadowcolor = v4{0.2f, 0.2f, 0.2f, 0.9f};
    string_op(String_Op_Draw, group, leftbottom+v3{2,-2,0}, str, font, shadowcolor);
    string_op(String_Op_Draw, group, leftbottom, str, font, color);
}

internal Rect2
string_rect(char *str, v3 left_bottom, Asset_Font *font) {
    return string_op(String_Op_Get_Rect, 0, left_bottom, str, font);
}

internal v2
string_dim(char *str, Asset_Font *font) {
    return get_dim(string_op(String_Op_Get_Rect, 0, {}, str, font));
}

internal Render_Group *
begin_render_group(Render_Commands *frame, u64 size) {
    Assert(frame->push_buffer_used + sizeof(Render_Group) + size <= frame->push_buffer_size);

    Render_Group *group = (Render_Group *)(frame->push_buffer_base + frame->push_buffer_used);
    frame->push_buffer_used += sizeof(Render_Group);
    group->capacity = size;
    group->base     = (frame->push_buffer_base + frame->push_buffer_used);
    group->used     = 0;
    frame->push_buffer_used += size;

    return group;
}
