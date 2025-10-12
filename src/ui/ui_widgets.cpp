/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



internal Ui_Signal
ui_label(Utf8 string)
{
    Ui_Box *box = ui_box_build_from_string(UI_BOX_FLAG_DRAW_BACKGROUND|UI_BOX_FLAG_DRAW_TEXT, string);
    Ui_Signal signal = ui_signal_from_box(box);
    return signal;
}

internal b32
ui_button(Utf8 text)
{
    Ui_Box_Flags flags = (UI_BOX_FLAG_MOUSE_CLICKABLE    | 
                          UI_BOX_FLAG_KEYBOARD_CLICKABLE |
                          UI_BOX_FLAG_DRAW_BACKGROUND    |
                          UI_BOX_FLAG_DRAW_HOT_EFFECT    |
                          UI_BOX_FLAG_DRAW_SHOOT_EFFECT  |
                          UI_BOX_FLAG_DRAW_TEXT);
    Ui_Box *box = ui_box_build_from_string(flags, text);
    Ui_Signal signal = ui_signal_from_box(box);
    return signal.pressed_left;
}

internal void
ui_div(void)
{
    ui_bg_push(V4(0.8f, 0.8f, 0.4f, 1.0f));
    {
        ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
        ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, 1.f);
        ui_box_build_from_key(UI_BOX_FLAG_DRAW_BACKGROUND, ui_key_zero);
    }
    ui_bg_pop();
}

internal b32
ui_drop(Utf8 text)
{
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_CHILDREN);
    Ui_Box_Flags flags = (UI_BOX_FLAG_MOUSE_CLICKABLE    | 
                          UI_BOX_FLAG_KEYBOARD_CLICKABLE |
                          UI_BOX_FLAG_DRAW_BACKGROUND    |
                          UI_BOX_FLAG_DRAW_HOT_EFFECT    |
                          UI_BOX_FLAG_DRAW_SHOOT_EFFECT  |
                          UI_BOX_FLAG_DRAW_TEXT);
    Ui_Box *box = ui_box_build_from_string(flags, text);

    Ui_Signal signal = ui_signal_from_box(box);
    if (signal.pressed_left)
    {
        box->on = !box->on;
    }
    return box->on;
}

internal void
ui_slider_f32(f32 *x, f32 lo, f32 hi, Utf8 text)
{
    ui_row()
    {
        // Slider
        ui_bg_push(v4{0.1f,0.0f,0.0f,1.0f});
        ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, ui_state->font_size*10.f);
        ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PCT, 1.f);
        Ui_Box *slider = ui_box_build_from_string(UI_BOX_FLAG_DRAW_BACKGROUND|
                                                  UI_BOX_FLAG_MOUSE_CLICKABLE,
                                                  utf8lit("slider"));
        ui_bg_pop();

        // Thumb
        ui_bg_push(v4{0.9f,0.9f,0.9f,1.0f});
        ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, ui_state->font_size);
        ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PCT, 1.f);
        f32 radius = ui_state->font_size*0.4f;
        ui_style_push(corner_radius00, radius); ui_style_push(corner_radius01, radius); ui_style_push(corner_radius10, radius); ui_style_push(corner_radius11, radius);
        Ui_Box *thumb = ui_box_build_from_string(UI_BOX_FLAG_DRAW_BACKGROUND|
                                                 UI_BOX_FLAG_DYNAMIC_POSITION,
                                                 utf8lit("thumb"));
        ui_style_pop(corner_radius00); ui_style_pop(corner_radius01); ui_style_pop(corner_radius10); ui_style_pop(corner_radius11);
        Ui_Signal thumb_signal = ui_signal_from_box(thumb);
        f32 t = map01(*x, lo, hi);
        f32 xl = slider->position[AXIS2_X];
        f32 xr = slider->position[AXIS2_X] + slider->computed_size[AXIS2_X] - thumb->computed_size[AXIS2_X];
        thumb->position[AXIS2_X] = lerp(xl, t, xr);
        thumb->position[AXIS2_Y] = slider->position[AXIS2_Y] + (slider->computed_size[AXIS2_Y] - thumb->computed_size[AXIS2_Y])*0.5f;
        ui_bg_pop();

        // Value Label
        ui_label(text);


        Ui_Signal slider_signal = ui_signal_from_box(slider);
        if (slider_signal.pressed_left || slider_signal.dragging_left)
        {
            thumb->position[AXIS2_X] = os->mouse_position_last.x - thumb->computed_size[AXIS2_X]*0.5f;
            thumb->position[AXIS2_X] = clamp(thumb->position[AXIS2_X], xl, xr);

            f32 t = map01(thumb->position[AXIS2_X], xl, xr);
            *x = lerp(lo, t, hi);
        }
    }
}

internal void
ui_platform_push(Utf8 text)
{
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_CHILDREN);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_CHILDREN);
    ui_flow_push(AXIS2_Y);

    {ui_style_push(corner_radius00, 8.f); ui_style_push(corner_radius01, 8.f); ui_style_push(corner_radius10, 8.f); ui_style_push(corner_radius11, 8.f); // @FIXME
        Ui_Box *platform = ui_box_build_from_string(UI_BOX_FLAG_DYNAMIC_POSITION,
                                                    utf8lit("platform"));
        ui_parent_push(platform);

        {ui_bg_push(v4{0.1f,0.1f,0.2f,1.0f}); ui_hot_bg_push(v4{0.2f,0.1f,0.1f,1.0f});
            Ui_Box *anchor = ui_box_build_from_string(UI_BOX_FLAG_MOUSE_CLICKABLE|
                                                      UI_BOX_FLAG_DRAW_BACKGROUND|
                                                      UI_BOX_FLAG_DRAW_TEXT      |
                                                      UI_BOX_FLAG_DRAW_HOT_EFFECT|
                                                      UI_BOX_FLAG_DRAW_ACTIVE_EFFECT,
                                                      text);
            Ui_Signal signal = ui_signal_from_box(anchor);

            if (signal.pressed_left)
            {
                anchor->mouse_position_last[AXIS2_X] = os->mouse_position_last.x;
                anchor->mouse_position_last[AXIS2_Y] = os->mouse_position_last.y;
            }

            if (signal.dragging_left)
            {
                platform->position[AXIS2_X] += (os->mouse_position_last.x - anchor->mouse_position_last[AXIS2_X]);
                platform->position[AXIS2_Y] += (os->mouse_position_last.y - anchor->mouse_position_last[AXIS2_Y]);

                platform->position[AXIS2_X] = clamp_lo(platform->position[AXIS2_X], 0.f);
                platform->position[AXIS2_Y] = clamp_lo(platform->position[AXIS2_Y], 0.f);

                anchor->mouse_position_last[AXIS2_X] = os->mouse_position_last.x;
                anchor->mouse_position_last[AXIS2_Y] = os->mouse_position_last.y;
            }
        }ui_bg_pop(); ui_hot_bg_pop();

    }ui_style_pop(corner_radius00); ui_style_pop(corner_radius01); ui_style_pop(corner_radius10); ui_style_pop(corner_radius11);

    ui_div();
}

internal void
ui_platform_pop(void)
{
    ui_parent_pop();
}

internal void
ui_row_push(void)
{
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_CHILDREN);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_CHILDREN);
    ui_flow_push(AXIS2_X);
    Ui_Box *row = ui_box_build_from_string(0, utf8lit("ASDASD"));
    ui_parent_push(row);
}
