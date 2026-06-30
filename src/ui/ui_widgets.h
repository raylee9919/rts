// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once



internal Ui_Signal ui_labelf(char *fmt, ...);
internal Ui_Signal ui_label(String string);

internal Ui_Signal ui_buttonf(char *fmt, ...);
internal Ui_Signal ui_button(String text);

#define ui_platform(text) defer_loop(ui_platform_push(text), ui_platform_pop())
internal void ui_platform_push(String text);
internal void ui_platform_pop(void);

#define ui_row() defer_loop(ui_row_push(), ui_parent_pop())
internal void ui_row_push(void);

#define ui_col() defer_loop(ui_col_push(), ui_parent_pop())
internal void ui_col_push(void);
