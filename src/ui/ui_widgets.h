#ifndef RTS_UI_BUILDER_H
#define RTS_UI_BUILDER_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */






internal Ui_Signal ui_labelf(char *fmt, ...);
internal Ui_Signal ui_label(Utf8 string);

internal Ui_Signal ui_buttonf(char *fmt, ...);
internal Ui_Signal ui_button(Utf8 text);

#define ui_platform(text) defer_loop(ui_platform_push(text), ui_platform_pop())
internal void ui_platform_push(Utf8 text);
internal void ui_platform_pop(void);

#define ui_row() defer_loop(ui_row_push(), ui_parent_pop())
internal void ui_row_push(void);

#define ui_col() defer_loop(ui_col_push(), ui_parent_pop())
internal void ui_col_push(void);

#endif // RTS_UI_BUILDER_H
