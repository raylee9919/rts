/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



internal void
ui_label(Utf8 string)
{
    {
        if (ui_style_top(flow) == AXIS2_X)
        {
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_TEXT);
        }
        else
        {
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
        }
        ui_size_push(AXIS2_Y, UI_SIZE_TYPE_TEXT);
    }

    {
        Ui_Box_Flags flags = (UI_BOX_FLAG_DRAW_BACKGROUND |
                              UI_BOX_FLAG_DRAW_TEXT);

        Ui_Box *box = ui_box_build_from_key(flags, ui_key_zero);
        ui_equip_text(box, string);
    }

    {
        ui_size_pop(AXIS2_X);
        ui_size_pop(AXIS2_Y);
    }
}

internal Ui_Signal
ui_button(Utf8 text)
{
    {
        if (ui_style_top(flow) == AXIS2_X)
        {
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_TEXT);
        }
        else
        {
            ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
        }
        ui_size_push(AXIS2_Y, UI_SIZE_TYPE_TEXT);
    }

    Ui_Box_Flags flags = (UI_BOX_FLAG_MOUSE_CLICKABLE    | 
                          UI_BOX_FLAG_KEYBOARD_CLICKABLE |
                          UI_BOX_FLAG_DRAW_BACKGROUND    |
                          UI_BOX_FLAG_DRAW_HOT_EFFECT    |
                          UI_BOX_FLAG_DRAW_TEXT);
    Ui_Box *box = ui_box_build_from_string(flags, text);
    ui_equip_text(box, text);

    {
        ui_size_pop(AXIS2_X);
        ui_size_pop(AXIS2_Y);
    }

    return ui_signal_from_box(box);
}


#define ui_pane(axis, string) defer_loop(ui_pane_push(axis, string), ui_pane_pop())
internal void
ui_pane_push(Axis2 axis, Utf8 string)
{
    ui_style_push(flow, axis);

    Ui_Box_Flags flags = UI_BOX_FLAG_DRAW_BACKGROUND;
    Ui_Box *box = ui_box_build_from_string(flags, string);
    ui_parent_push(box);

    ui_label(string);

    ui_style_push(bg, V4(0.8f, 0.8f, 0.5f, 1.0f));
    ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_PX, 1.f);
    Ui_Box *line = ui_box_build_from_key(UI_BOX_FLAG_DRAW_BACKGROUND, ui_key_zero);
    ui_size_pop(AXIS2_Y);
    ui_size_pop(AXIS2_X);
    ui_style_pop(bg);
}

internal void
ui_pane_pop(void)
{
    ui_parent_pop();

    ui_style_pop(flow);
}


