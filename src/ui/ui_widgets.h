#ifndef RTS_UI_BUILDER_H
#define RTS_UI_BUILDER_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */





internal Ui_Signal ui_label(Utf8 string);
internal Ui_Signal ui_button(Utf8 text);



#define ui_col_named(string) defer_loop(ui_col_named_push(string), ui_col_pop())
#define ui_col() defer_loop(ui_col_push(), ui_col_pop())
internal void ui_col_named_push(Utf8 string);
internal void ui_col_push(void);
internal void ui_col_pop(void);

#define ui_row_named(string) defer_loop(ui_row_named_push(string), ui_row_pop())
#define ui_row() defer_loop(ui_row_push(), ui_row_pop())
internal void ui_row_push(void);
internal void ui_row_pop(void);

#endif // RTS_UI_BUILDER_H
