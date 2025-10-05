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
    Ui_Box *box = ui_box_build_from_string(UI_BOX_FLAG_DRAW_TEXT, string);
    Ui_Signal signal = ui_signal_from_box(box);
    return signal;
}

internal Ui_Signal
ui_button(Utf8 text)
{
    Ui_Box_Flags flags = (UI_BOX_FLAG_MOUSE_CLICKABLE    | 
                          UI_BOX_FLAG_KEYBOARD_CLICKABLE |
                          UI_BOX_FLAG_DRAW_BACKGROUND    |
                          UI_BOX_FLAG_DRAW_HOT_EFFECT    |
                          UI_BOX_FLAG_DRAW_ACTIVE_EFFECT |
                          UI_BOX_FLAG_DRAW_SHOOT_EFFECT  |
                          UI_BOX_FLAG_DRAW_TEXT);
    Ui_Box *box = ui_box_build_from_string(flags, text);
    Ui_Signal signal = ui_signal_from_box(box);
    return signal;
}

internal Ui_Signal
ui_radio(Utf8 text)
{
    Ui_Signal signal = {};

    ui_size_push(AXIS2_X, UI_SIZE_TYPE_PCT, 1.f);
    ui_size_push(AXIS2_Y, UI_SIZE_TYPE_CHILDREN);
    ui_row_named(text)
    {
        {
            Ui_Box_Flags flags = (UI_BOX_FLAG_MOUSE_CLICKABLE    | 
                                  UI_BOX_FLAG_KEYBOARD_CLICKABLE |
                                  UI_BOX_FLAG_DRAW_BACKGROUND    |
                                  UI_BOX_FLAG_DRAW_HOT_EFFECT    |
                                  UI_BOX_FLAG_DRAW_ACTIVE_EFFECT |
                                  UI_BOX_FLAG_DRAW_SHOOT_EFFECT  |
                                  UI_BOX_FLAG_DRAW_TEXT);
            Ui_Box *radio = ui_box_build_from_string(flags, utf8lit("[O]"));
            signal = ui_signal_from_box(radio);
        }
        {
            Ui_Box_Flags flags = (UI_BOX_FLAG_DRAW_BACKGROUND |
                                  UI_BOX_FLAG_DRAW_TEXT);
            Ui_Box *label = ui_box_build_from_string(flags, text);
        }
    }
    ui_size_pop(AXIS2_X);
    ui_size_pop(AXIS2_Y);

    return signal;
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
}

internal void
ui_pane_pop(void)
{
    ui_parent_pop();

    ui_style_pop(flow);
}


// @Note: Column
//
internal void
ui_col_named_push(Utf8 string)
{
    ui_style_push(flow, AXIS2_Y);
    ui_parent_push(ui_box_build_from_string(0, string));
}

internal void
ui_col_push(void)
{
    ui_col_named_push(utf8lit(""));
}

internal void
ui_col_pop(void)
{
    ui_parent_pop();
    ui_style_pop(flow);
}

// @Note: Row
//
internal void
ui_row_named_push(Utf8 string)
{
    ui_style_push(flow, AXIS2_X);
    ui_parent_push(ui_box_build_from_string(0, string));
}

internal void
ui_row_push(void)
{
    ui_row_named_push(utf8lit(""));
}

internal void
ui_row_pop(void)
{
    ui_parent_pop();
    ui_style_pop(flow);
}
