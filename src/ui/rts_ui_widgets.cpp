/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */




internal void
ui_text(Utf8 text)
{
    Ui_Box_Flags flags = (UI_BOX_FLAG_DRAW_BACKGROUND |
                          UI_BOX_FLAG_DRAW_TEXT);

    Ui_Box *box = ui_push(text, flags);
    {
        box->semantic_size[0].type = UI_SIZE_TYPE_TEXT;
        box->semantic_size[1].type = UI_SIZE_TYPE_TEXT;
    }
    ui_pop();
}

#define ui_pane_x(text) defer_loop(ui_pane_x_push(text), ui_pop())
#define ui_pane_y(text) defer_loop(ui_pane_y_push(text), ui_pop())
#define ui_pane_x_push(text) ui_pane_push(text, UI_BOX_FLAG_FLOW_X)
#define ui_pane_y_push(text) ui_pane_push(text, UI_BOX_FLAG_FLOW_Y)
internal void
ui_pane_push(Utf8 text, Ui_Box_Flags axis)
{
    Ui_Box_Flags flags = axis; 

    Ui_Box *box = ui_push(text, flags);
    {
        box->semantic_size[0].type = UI_SIZE_TYPE_CHILDREN;
        box->semantic_size[1].type = UI_SIZE_TYPE_CHILDREN;
    }
}


internal Ui_Signal
ui_button(Utf8 text)
{
    // Signal
    //
    Ui_Key key = ui_key_from_string(text);
    Ui_Box *box = ui_box_from_key(key);
    Ui_Signal result = ui_signal_from_box(box); // Safe to pass NULL.

    // Build
    //
    Ui_Box_Flags flags = (UI_BOX_FLAG_MOUSE_CLICKABLE | 
                          UI_BOX_FLAG_KEYBOARD_CLICKABLE |
                          UI_BOX_FLAG_DRAW_BACKGROUND |
                          UI_BOX_FLAG_DRAW_TEXT);

    box = ui_push(text, flags);
    {
        box->semantic_size[0].type = UI_SIZE_TYPE_TEXT;
        box->semantic_size[1].type = UI_SIZE_TYPE_TEXT;
    }
    ui_pop();

    return result;
}
