// Copyright Seong Woo Lee. All Rights Reserved.


internal Ui_Signal
ui_labelf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Utf8 text = utf8fv(ui_build_arena(), fmt, args);
    Ui_Signal result = ui_label(text);
    va_end(args);
    return result;
}

internal Ui_Signal
ui_label(Utf8 string)
{
    Ui_Box *box = ui_box_build_from_string(UI_BOX_FLAG_DRAW_TEXT, string);
    Ui_Signal signal = ui_signal_from_box(box);
    return signal;
}

internal Ui_Signal
ui_buttonf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Utf8 text = utf8fv(ui_build_arena(), fmt, args);
    Ui_Signal result = ui_button(text);
    va_end(args);
    return result;
}

internal Ui_Signal 
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
    return signal;
}

internal b32
ui_expander(Utf8 text)
{
    Ui_Box *box = &ui_nil_box;


    ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_CHILDREN);
    ui_bg_push(v4{0.3f,0.3f,0.5f,1.0f});
    Ui_Box *expander = ui_box_build_from_stringf(UI_BOX_FLAG_MOUSE_CLICKABLE    |
                                                 UI_BOX_FLAG_KEYBOARD_CLICKABLE |
                                                 UI_BOX_FLAG_DRAW_BACKGROUND    |
                                                 UI_BOX_FLAG_DRAW_HOT_EFFECT,
                                                 "%S__Expander", text);
    ui_bg_pop();
    Ui_Signal signal = ui_signal_from_box(expander);
    if (signal.pressed_left) 
    {
        expander->on = !expander->on; 
    }

    ui_flow_push(AXIS2_X);
    ui_parent_push(expander);
    {
        ui_label(expander->on ? utf8lit("▼") : utf8lit("▶"));
        ui_label(text);
    }
    ui_parent_pop();

    return expander->on;
}

internal void
ui_slider_f32(f32 *x, f32 lo, f32 hi, Utf8 text)
{
    ui_col()
    {
        // Spacing Y
        ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, 0.f);
        ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, 2.f);
        ui_box_build_from_key(0, ui_key_zero);
        ui_row()
        {
            // Spacing X
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, 2.f);
            ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, 0.f);
            ui_box_build_from_key(0, ui_key_zero);

            // Slider
            f32 slider_size_x = ui_state->font_size*10.f;
            f32 radius = ui_state->font_size*0.4f;
            ui_style_push(corner_radius00, radius); ui_style_push(corner_radius01, radius); ui_style_push(corner_radius10, radius); ui_style_push(corner_radius11, radius);
            ui_bg_push(v4{0.2f,0.2f,0.2f,1.0f});
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, slider_size_x);
            ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PCT, 1.f);
            Ui_Box *slider = ui_box_build_from_stringf(UI_BOX_FLAG_DRAW_BACKGROUND|
                                                       UI_BOX_FLAG_MOUSE_CLICKABLE,
                                                       "%S_slider", text);
            ui_bg_pop();

            // Thumb
            f32 thumb_size_x = ui_state->font_size*1.f;
            ui_bg_push(v4{0.3f,0.3f,0.3f,1.0f});
            ui_hot_bg_push(v4{0.2f,0.2f,0.2f,1.0f});
            ui_active_bg_push(v4{0.5f,0.5f,1.0f,1.0f});
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, thumb_size_x);
            ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PCT, 1.f);
            Ui_Box *thumb = ui_box_build_from_stringf(UI_BOX_FLAG_DRAW_BACKGROUND|
                                                      UI_BOX_FLAG_MOUSE_CLICKABLE|
                                                      UI_BOX_FLAG_DRAW_HOT_EFFECT|
                                                      UI_BOX_FLAG_DRAW_ACTIVE_EFFECT|
                                                      UI_BOX_FLAG_DYNAMIC_POSITION,
                                                      "%S_thumb", text);
            ui_style_pop(corner_radius00); ui_style_pop(corner_radius01); ui_style_pop(corner_radius10); ui_style_pop(corner_radius11);
            ui_active_bg_pop();
            ui_hot_bg_pop();
            ui_bg_pop();

            // Adjust thumb position according to the target data.
            if (thumb->first_tick != ui_state->tick_current)
            {
                f32 t = map_unorm(*x, lo, hi);
                f32 xl = slider->position[AXIS2_X];
                f32 xr = slider->position[AXIS2_X] + slider_size_x - thumb_size_x;
                thumb->position[AXIS2_X] = lerp(xl, t, xr);

                Ui_Signal thumb_signal = ui_signal_from_box(thumb);

                if (thumb_signal.pressed_left)
                {
                    thumb->mouse_position_last[AXIS2_X] = ui_state->current_mouse_position.x;
                }

                if (thumb_signal.dragging_left)
                {
                    f32 delta = ui_state->current_mouse_position.x - thumb->mouse_position_last[AXIS2_X];
                    thumb->position[AXIS2_X] += delta;
                    thumb->mouse_position_last[AXIS2_X] = ui_state->current_mouse_position.x;
                }

                Ui_Signal slider_signal = ui_signal_from_box(slider);
                f32 thumb_cen_x = thumb->position[AXIS2_X] + thumb_size_x*0.5f;
                if (ui_key_match(ui_state->active_key, slider->key) && (abs(ui_state->current_mouse_position.x - thumb_cen_x) > 1.f))
                {
                    f32 delta = (ui_state->current_mouse_position.x > thumb_cen_x) ? 1.f : -1.f;
                    thumb->position[AXIS2_X] += delta;
                }

                // Finalize thumb position.
                thumb->position[AXIS2_X] = clamp(thumb->position[AXIS2_X], xl, xr);
                thumb->position[AXIS2_Y] = slider->position[AXIS2_Y] + (slider->computed_size[AXIS2_Y] - thumb->computed_size[AXIS2_Y])*0.5f;

                // Thumb position -> Write to data.
                t = map_unorm(thumb->position[AXIS2_X], xl, xr);
                *x = lerp(lo, t, hi);

            }
            else
            {
                thumb->flags &=(~UI_BOX_FLAG_DRAW_BACKGROUND);
            }


            // Spacing X
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_PX, 8.f);
            ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, 0.f);
            ui_box_build_from_key(0, ui_key_zero);


            // Value Label
            Ui_Box *value_label= ui_box_build_from_stringf(UI_BOX_FLAG_DYNAMIC_POSITION|
                                                           UI_BOX_FLAG_DRAW_TEXT,
                                                           "%.2f###%S__SliderValueLabel", *x, text);
            value_label->position[AXIS2_X] = slider->position[AXIS2_X] + (slider_size_x - value_label->computed_size[AXIS2_X]) * 0.5f;
            value_label->position[AXIS2_Y] = slider->position[AXIS2_Y];


            // Text
            ui_box_build_from_string(UI_BOX_FLAG_DRAW_TEXT, text);
        }
    }
}

internal void
ui_platform_push(Utf8 text)
{
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_CHILDREN);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_CHILDREN);
    ui_flow_push(AXIS2_Y);

    {ui_style_push(corner_radius00, 8.f); ui_style_push(corner_radius01, 8.f); ui_style_push(corner_radius10, 8.f); ui_style_push(corner_radius11, 8.f);
        Ui_Box *platform = ui_box_build_from_string(UI_BOX_FLAG_DRAW_BACKGROUND|
                                                    UI_BOX_FLAG_DYNAMIC_POSITION,
                                                    utf8lit("platform"));
        ui_parent_push(platform);

        {ui_bg_push(v4{0.3f,0.3f,0.5f,1.0f}); ui_hot_bg_push(v4{0.4f,0.4f,0.7f,1.0f}); ui_active_bg_push(v4{0.1f,0.1f,0.3f,1.0f});
            Ui_Box *anchor = ui_box_build_from_string(UI_BOX_FLAG_MOUSE_CLICKABLE|
                                                      UI_BOX_FLAG_DRAW_BACKGROUND|
                                                      UI_BOX_FLAG_DRAW_TEXT      |
                                                      UI_BOX_FLAG_DRAW_HOT_EFFECT|
                                                      UI_BOX_FLAG_DRAW_ACTIVE_EFFECT,
                                                      text);
            Ui_Signal signal = ui_signal_from_box(anchor);

            if (signal.pressed_left)
            {
                anchor->mouse_position_last[AXIS2_X] = ui_state->current_mouse_position.x;
                anchor->mouse_position_last[AXIS2_Y] = ui_state->current_mouse_position.y;
            }

            if (signal.dragging_left)
            {
                platform->position[AXIS2_X] += (ui_state->current_mouse_position.x - anchor->mouse_position_last[AXIS2_X]);
                platform->position[AXIS2_Y] += (ui_state->current_mouse_position.y - anchor->mouse_position_last[AXIS2_Y]);

                platform->position[AXIS2_X] = clamp(platform->position[AXIS2_X], 0.f, ui_state->root->computed_size[AXIS2_X] - anchor->computed_size[AXIS2_X]);
                platform->position[AXIS2_Y] = clamp(platform->position[AXIS2_Y], 0.f, ui_state->root->computed_size[AXIS2_Y] - anchor->computed_size[AXIS2_Y]);

                anchor->mouse_position_last[AXIS2_X] = ui_state->current_mouse_position.x;
                anchor->mouse_position_last[AXIS2_Y] = ui_state->current_mouse_position.y;
            }
        }ui_bg_pop(); ui_hot_bg_pop(); ui_active_bg_pop();

    }ui_style_pop(corner_radius00); ui_style_pop(corner_radius01); ui_style_pop(corner_radius10); ui_style_pop(corner_radius11);

    ui_bg_push(V4(0.8f, 0.8f, 0.4f, 1.0f));
    {
        ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
        ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, 1.f);
        ui_box_build_from_key(UI_BOX_FLAG_DRAW_BACKGROUND, ui_key_zero);
    }
    ui_bg_pop();
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
    Ui_Box *row = ui_box_build_from_key(0, ui_key_zero);
    ui_parent_push(row);
}

internal void
ui_col_push(void)
{
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_CHILDREN);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_CHILDREN);
    ui_flow_push(AXIS2_Y);
    Ui_Box *col = ui_box_build_from_key(0, ui_key_zero);
    ui_parent_push(col);
}
