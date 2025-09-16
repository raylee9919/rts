#ifndef RTS_CONSOLE_H
#define RTS_CONSOLE_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define CONSOLE_TARGET_T 0.2f

struct Console_Cursor
{
    v2 offset;
    v2 dim;
    v4 color1;
    v4 color2;
};

// @Temporary:
struct Console
{
    b32 initted;

    f32         dt;
    v2          half_dim;
    v4          bg_color;

    Asset_Font  *font;
    v4          text_color;

    Console_Cursor cursor;

    char        cbuf[64];
    u32         cbuf_at;
    v2          input_baseline_offset;
    
    b32 is_down;
};


internal void console_init(Console *console, Asset_Font *font);
internal b32 console_cmd_equal(Console *console, char *str);
internal void console_draw(Console *console, Render_Group *group, f32 real_time, f32 width, f32 height);
internal void console_update(Console *console, Game_State *game_state, Input *input);

#endif // RTS_CONSOLE_H
