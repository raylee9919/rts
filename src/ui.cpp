/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define UI_BORDER_SIZE 3
#define UI_SPACE 0
#define UI_FADEOUT_TEXT_SET_TIME 1.0f

internal u32
ui_hash(s32 id)
{
    return id*2+id*2+id*6; // @Better hash function. -Ray
}

internal void
generate_crc32_lut(u32 *dst)
{
    u32 polynomial = 0xEDB88320;

    for (u32 i = 0; i < 256; ++i) {
        u32 crc = i;
        for (u32 j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
        dst[i] = crc;
    }
}

internal u32
crc32_hash(const char *str, u32 *lut)
{
    u32 crc = 0xffffffff;
    umm length = string_length(str);

    for (const char *c = str; c < str + length; ++c) {
        crc = ((crc >> 8) ^ lut[(crc ^ *c) & 0xFF]);
    }

    crc ^= 0xffffffff;
    return crc;
}

void Ui::init(Input *input_, Arena *arena_, Render_Group *render_group_, Asset_Font *font_, Asset_Font *bigfont_) 
{
    initted = true;

    input = input_;
    mouse = &input->mouse;
    arena = arena_;
    render_group = render_group_;
    font = font_;
    bigfont = bigfont_;

    top_left = V2(-1, 1);

    generate_crc32_lut(crc32_lut);
}

b32 Ui::hot_begins(Rect2 rect)
{
    return (active == 0 && !mouse->is_down[Mouse_Left] && in_rect(rect, mouse->position));
}

void Ui::begin(char *label, v2 top_left_, b32 attachanchor)
{
    if (attachanchor) {
        u32 id = crc32_hash(label, crc32_lut);
        umm index = ui_hash(id) % array_count(hashmap.entries);
        Ui_Element *element = hashmap.entries + index;
        for (;;) {
            if (element->id == id) {
                top_left = element->position;
                break;
            } else if (!element->next) {
                Ui_Element *newelement = element->next = push_struct(arena, Ui_Element);
                newelement->id = id;
                newelement->position = top_left = top_left_;
                newelement->next = 0;
                break;
            } else {
                element = element->next;
            }
        }

        v2 o = hadamard(top_left * 0.5f + V2(0.5f), input->draw_dim);

        f32 anchorsize = 25;
        v4 anchorcolor = V4(0.8f, 0.8f, 0.6f, 0.5f);
        Rect2 anchor;
        anchor.min = o + V2(0, -anchorsize);
        anchor.max = o + V2(anchorsize, 0);

        if (active == id) {
            input->interacted_ui = true;
            if (!mouse->is_down[Mouse_Left] && mouse->toggle[Mouse_Left]) {
                active = 0;
            } else {
                v2 dmouse = mouse->position - input->prev_mouse_p;
                o += dmouse;
                element->position = V2(map01_binormal(o.x, 0, input->draw_dim.x), map01_binormal(o.y, 0, input->draw_dim.y));
                anchorcolor.rgb *= 0.5f;
            }
        } else if (hot == id) { 
            if (mouse->is_down[Mouse_Left] && in_rect(anchor, mouse->position)) {
                active = id;
                anchorcolor.rgb *= 0.8f;
            } else if (!in_rect(anchor, mouse->position)) {
                hot = 0;
            } else {
                anchorcolor.rgb *= 1.5f;
            }
        } else {
            if (hot_begins(anchor)) {
                hot = id;
                anchorcolor.rgb *= 1.5f;
            }
        }

        push_bordered_rect(render_group, anchor, 0, anchorcolor, 2.0f, V4(V3(0.2f), 0.5f));

        top_left.y -= ((anchorsize + 2) * 2.0f / input->draw_dim.y);
    } else {
        top_left = top_left_;
    }
}

void Ui::end()
{
    top_left = V2(-1, 1);
}

void Ui::next(f32 itemheight)
{
    top_left.y -= ((itemheight + UI_SPACE) * 2.0f / input->draw_dim.y);
}

void Ui::description(char *description)
{
    if (description) {
        f32 from_bottom = 10.0f;
        f32 padding = 2.0f;
        v2 dim = string_dim(description, font);
        push_shadowed_string(render_group, V3((input->draw_dim.x - dim.x)*0.5f, from_bottom + padding, 1), description, font, V4(V3(0.7f), 1.0f));
    }
}

void Ui::text(v4 color, char *text)
{
    u32 id = crc32_hash(text, crc32_lut);

    v2 o = hadamard(top_left * 0.5f + V2(0.5f), input->draw_dim);
    v2 text_dim = string_dim(text, font);
    f32 h = font->v_advance;
    f32 p = (h - text_dim.y) * 0.5f;
    f32 b = UI_BORDER_SIZE;
    v2 textbox_dim = text_dim + V2(2*p);

    Rect2 textbox_rect = Rect2{
        o + V2(b, -textbox_dim.y), 
          o + V2(b + textbox_dim.x)
    };

    Rect2 container_rect = Rect2{
        V2(o.x, o.y - (b + b + textbox_dim.y)),
            V2(o.x + b + textbox_dim.x + b, o.y),
    };

    push_bordered_rect(render_group, container_rect, 0, color, b, V4(V3(0.2f), color.a));
    string_op(String_Op_Draw, render_group, V3(container_rect.min + V2(b+p), 1), text, font, RGBA_WHITE);
    push_shadowed_string(render_group, V3(container_rect.min + V2(b+p), 1), text, font, RGBA_WHITE);

    next(textbox_dim.y + 2*b);
}

void Ui::fadeout_text(v4 color, char *text)
{
    fadeout_texts[next_fadeout_text].t = UI_FADEOUT_TEXT_SET_TIME;
    fadeout_texts[next_fadeout_text].text = text;
    fadeout_texts[next_fadeout_text].color = color;
    next_fadeout_text = (next_fadeout_text + 1) % array_count(fadeout_texts);
}

b32 Ui::button(v4 color, char *text, char *desc)
{
    b32 result = false;

    v4 button_color = color;
    u32 id = crc32_hash(text, crc32_lut);

    v2 o = hadamard(top_left * 0.5f + V2(0.5f), input->draw_dim);
    v2 text_dim = string_dim(text, font);
    f32 h = font->v_advance;
    f32 p = (h - text_dim.y) * 0.5f;
    f32 b = UI_BORDER_SIZE;
    v2 textbox_dim = text_dim + V2(2*p);

    Rect2 textbox_rect = Rect2{
        o + v2{b, -textbox_dim.y - b}, 
          o + v2{b + textbox_dim.x, -b}
    };
    Rect2 textrect = add_radius_to(textbox_rect, V2(-p));

    Rect2 container_rect = Rect2{
        V2(o.x, o.y - (2*b + textbox_dim.y)),
            V2(o.x + 2*b + textbox_dim.x, o.y),
    };

    if (active == id) {
        input->interacted_ui = true;
        description(desc);
        if (!mouse->is_down[Mouse_Left] && mouse->toggle[Mouse_Left]) {
            active = 0;
            if (in_rect(container_rect, mouse->position)) {
                result = true;
            } else {
                result = false;
            }
        } else {
            button_color.rgb *= 0.5f;
        }
    } else if (hot == id) { 
        description(desc);
        if (mouse->is_down[Mouse_Left] && in_rect(container_rect, mouse->position)) {
            active = id;
            button_color.rgb *= 0.8f;
        } else if (!in_rect(container_rect, mouse->position)) {
            hot = 0;
        } else {
            button_color.rgb *= 1.5f;
        }
    } else {
        if (hot_begins(container_rect)) {
            hot = id;
            button_color.rgb *= 1.5f;
        }
    }

    push_bordered_rect(render_group, container_rect, 0, button_color, b, V4(V3(0.2f), button_color.a));
    push_shadowed_string(render_group, V3(textrect.min, 1), text, font, RGBA_WHITE);

    next(h+2*b);

    return result;
}

b32 Ui::checkbox(b32 *data, v4 default_color, char *text, char *desc)
{
    b32 result = false;

    v4 color = default_color;
    u32 id = crc32_hash(text, crc32_lut);

    v2 o = hadamard(top_left * 0.5f + V2(0.5f), input->draw_dim);
    v2 text_dim = string_dim(text, font);
    f32 h = font->v_advance;
    f32 p = (h - text_dim.y) * 0.5f;
    f32 b = UI_BORDER_SIZE;
    f32 m = 2.0f;
    f32 space = 2.0f;

    v2 checkbox_dim = V2(h);
    v2 textbox_dim = text_dim + V2(2*p);

    Rect2 checkbox_rect;
    checkbox_rect.min = o - V2(-b, h + b);
    checkbox_rect.max = o + V2(h + b, -b);

    Rect2 textbox_rect;
    textbox_rect.min = checkbox_rect.min + V2(m + checkbox_dim.x, 0);
    textbox_rect.max = checkbox_rect.max + V2(m + textbox_dim.x, 0);

    Rect2 text_rect;
    text_rect.min = textbox_rect.min + V2(p);
    text_rect.max = textbox_rect.max - V2(p);

    Rect2 container_rect;
    container_rect.min = checkbox_rect.min - V2(b);
    container_rect.max = textbox_rect.max + V2(b);

    if (active == id) {
        input->interacted_ui = true;
        description(desc);
        if (!mouse->is_down[Mouse_Left] && mouse->toggle[Mouse_Left]) {
            active = 0;
            if (in_rect(container_rect, mouse->position)) {
                *data = !*data;
            }
        } else {
            color.rgb *= 0.5f;
        }
    } else if (hot == id) { 
        description(desc);
        if (mouse->is_down[Mouse_Left] && in_rect(container_rect, mouse->position)) {
            active = id;
            color.rgb *= 0.8f;
        } else if (!in_rect(container_rect, mouse->position)) {
            hot = 0;
        } else {
            color.rgb *= 1.5f;
        }
    } else {
        if (hot_begins(container_rect)) {
            hot = id;
            color.rgb *= 1.5f;
        }
    }

    push_bordered_rect(render_group, container_rect, 0, color, b, V4(V3(0.2f), color.a));
    push_rect(render_group, checkbox_rect, 0, V4(V3(1.0f), 0.7f));
    if (*data) {
        v2 offset = checkbox_dim*0.2f;
        Rect2 rect = {checkbox_rect.min + offset, checkbox_rect.max - offset};
        push_rect(render_group, rect, 0, V4(V3(0.00f,0.50f,1.00f), 0.5f));
    }
    push_shadowed_string(render_group, V3(text_rect.min, 1), text, font, RGBA_WHITE);

    next(checkbox_dim.y + 2*b);

    result = *data;

    return result;
}

b32 Ui::slider(f32 *data, f32 min, f32 max, v4 default_color, char *text, char *desc)
{
    b32 result = false;

    v4 color = default_color;
    u32 id = crc32_hash(text, crc32_lut);

    v2 o = hadamard(top_left * 0.5f + V2(0.5f), input->draw_dim);
    v2 text_dim = string_dim(text, font);
    f32 h = font->v_advance;
    f32 p = (h - text_dim.y) * 0.5f;

    f32 b = UI_BORDER_SIZE;
    f32 m = 2.0f;
    f32 space = 2.0f;

    v2 slider_dim = V2(0.5f*h, h);
    v2 raildim = V2(200.0f, h);
    v2 textbox_dim = text_dim + V2(2*p);

    Rect2 textbox_rect;
    textbox_rect.min = o + V2(b, -textbox_dim.y); 
    textbox_rect.max = o + V2(b + textbox_dim.x);

    Rect2 text_rect;
    text_rect.min = textbox_rect.min + V2(p);
    text_rect.max = textbox_rect.max - V2(p);

    f32 slider_minx = o.x + b + textbox_dim.x + space + slider_dim.x*0.5f;
    f32 slider_maxx = o.x + b + textbox_dim.x + space + raildim.x - slider_dim.x*0.5f;

    f32 t = map01(*data, min, max);
    f32 slider_center_x = lerp(slider_minx, t, slider_maxx);
    f32 slider_center_y = o.y - (b + p + text_dim.y*0.5f);
    Rect2 slider_rect = Rect2{
        V2(slider_center_x - slider_dim.x*0.5f, slider_center_y - slider_dim.y*0.5f),
            V2(slider_center_x + slider_dim.x*0.5f, slider_center_y + slider_dim.y*0.5f)
    };

    Rect2 rail_rect = Rect2{
        V2(o.x + b + textbox_dim.x + space, o.y - (b + textbox_dim.y)),
            V2(o.x + b + textbox_dim.x + space + raildim.x, o.y - b)
    };

    Rect2 container_rect = Rect2{
        V2(o.x, o.y - (b + b + textbox_dim.y)),
            V2(o.x + b + textbox_dim.x + space + raildim.x + b, o.y),
    };

    v4 slidercolor = V4(V3(0.8f), 0.9f);
    f32 sliderspeed = 3.0f;

    if (active == id) {
        input->interacted_ui = true;
        description(desc);
        if (!mouse->is_down[Mouse_Left] && mouse->toggle[Mouse_Left]) {
            active = 0;
        } else {
            f32 dmousex = mouse->position.x - input->prev_mouse_p.x;
            f32 ndmousex = dmousex / input->draw_dim.x;
            f32 dval = (max-min)*ndmousex;
            *data = clamp(*data + dval*sliderspeed, min, max);
            slidercolor.rgb *= 1.5f;
        }
    } else if (hot == id) { 
        description(desc);
        if (mouse->is_down[Mouse_Left] && in_rect(slider_rect, mouse->position)) {
            active = id;
            slidercolor.rgb *= 0.8f;
        } else if (!in_rect(slider_rect, mouse->position)) {
            hot = 0;
        } else {
            slidercolor.rgb *= 0.8f;
        }
    } else {
        if (hot_begins(slider_rect)) {
            hot = id;
            slidercolor.rgb *= 0.8f;
        }
    }

    char value[256];
    snprintf(value, sizeof(value), "%.6f", *data);
    v2 value_text_dim = string_dim(value, font);
    Rect2 value_text_rect = Rect2{
        V2(o.x + b + textbox_dim.x + space + raildim.x*0.5f - value_text_dim.x*0.5f, o.y - (b + textbox_dim.y*0.5f + value_text_dim.y*0.5f)),
            V2(o.x + b + textbox_dim.x + space + raildim.x*0.5f + value_text_dim.x*0.5f, o.y - (b + textbox_dim.y*0.5f - value_text_dim.y*0.5f))
    };

    push_bordered_rect(render_group, container_rect, 0, color, b, V4(V3(0.2f), color.a));
    push_rect(render_group, rail_rect, 0, V4(V3(0.1f), 0.4f));
    push_rect(render_group, slider_rect, 0, slidercolor);
    push_shadowed_string(render_group, V3(text_rect.min, 1), text, font, RGBA_WHITE);
    push_shadowed_string(render_group, V3(value_text_rect.min, 1), value, font, RGBA_WHITE);

    next(textbox_dim.y + 2*b);

    result = *data;

    return result;
}

void Ui::gizmo(v3 *position, m4x4 view_proj)
{
    v2 o = hadamard(binormal_to_normal(project(*position, view_proj).xy), input->draw_dim);

    f32 bordersize = 2.0f;

    Rect2 anchor_rect;
    v2 anchorsize = v2{8,8};
    anchor_rect.min = o - anchorsize;
    anchor_rect.max = o + anchorsize;
    push_bordered_rect(render_group, anchor_rect, 0, RGBA_WHITE, bordersize, V4(V3(0.2f), 1.0f));

    f32 len = 100.0f;
    v2 endsize = v2{10,10};
    v3 axis[3] = {v3{1,0,0}, v3{0,1,0}, v3{0,0,1}};
    f32 alpha = 1.0f;
    v4 colors[3] = {v4{0.91f,0.26f,0.35f,alpha}, v4{0.49f,0.83f,0.28f,alpha}, v4{0.00f,0.29f,0.70f,alpha}};
    u32 ids[3] = {
        crc32_hash("gizmo_x", crc32_lut),
        crc32_hash("gizmo_y", crc32_lut),
        crc32_hash("gizmo_z", crc32_lut),
    };

    for (u32 i = 0; i < 3; ++i) {
        v3 axis_p = *position + axis[i];
        v2 axis_d = normalize(hadamard(binormal_to_normal(project(axis_p, view_proj).xy), input->draw_dim) - o);
        v2 axis_o = o + len * axis_d;

        Rect2 rect;
        rect.min = axis_o - endsize;
        rect.max = axis_o + endsize;

        u32 id = ids[i];
        v4 color = colors[i];

        if (active == id) {
            input->interacted_ui = true;
            if (!mouse->is_down[Mouse_Left] && mouse->toggle[Mouse_Left]) {
                active = 0;
            } else {
                v2 dmouse = mouse->position - input->prev_mouse_p;
                f32 sign = 1.0f;
                if (dot(axis_d, dmouse) < 0.0f) sign = -sign;
                f32 d = sign * length(dmouse) * 0.005f;

                *position += d*axis[i];
                color.rgb *= 0.8f;
            }
        } else if (hot == id) { 
            if (mouse->is_down[Mouse_Left] && in_rect(rect, mouse->position)) {
                active = id;
                color.rgb *= 0.8f;
            } else if (!in_rect(rect, mouse->position)) {
                hot = 0;
            } else {
                color.rgb *= 1.5f;
            }
        } else {
            if (hot_begins(rect)) {
                hot = id;
                color.rgb *= 1.5f;
            }
        }

        push_bordered_rect(render_group, rect, 0, color, bordersize, V4(V3(0.2f), 1.0f));
    }
}

void Ui::end_frame()
{
    for (u32 i = 0; i < array_count(fadeout_texts); ++i) {
        Ui_Fadeout_Text *fadeout = fadeout_texts + i;
        if (fadeout->t > 0.0f) {
            f32 t = map01(fadeout->t, 0.0f, UI_FADEOUT_TEXT_SET_TIME);
            fadeout->t -= input->dt;
            v2 screencenter = hadamard(V2(0.5f), input->draw_dim);
            v2 textdim = string_dim(fadeout->text, bigfont);
            v2 bottomleft = screencenter - textdim*0.5f;
            bottomleft.y += lerp(input->draw_dim.y*0.25f, t, input->draw_dim.y*0.3f);
            f32 alpha = lerp(0.0f, t, fadeout->color.a);
            string_op(String_Op_Draw, render_group, V3(bottomleft + V2(8.0f, -8.0f), 1.0f), fadeout->text, bigfont, V4(V3(0.2f), alpha));
            string_op(String_Op_Draw, render_group, V3(bottomleft, 1.0f), fadeout->text, bigfont, V4(fadeout->color.rgb, alpha));
        }
    }
}
