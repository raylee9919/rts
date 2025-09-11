/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */

#define CONSOLE_TARGET_T 0.2f

struct Console_Cursor
{
    v2 offset;
    v2 dim;
    v4 color1;
    v4 color2;
};

// @Temporary
struct Console
{
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

    b32 initted;
};

internal void
init_console(Console *console, Asset_Font *font) //@TODO: assert font...
{
    if (!console->initted) {
        console->initted = true;

        console->half_dim   = v2{600.f, 250.0f};
        console->dt         = 0.0f;
        console->bg_color   = v4{0.02f, 0.02f, 0.02f, 0.9f};
        console->font       = font;
        console->text_color = v4{1.0f, 1.0f, 1.0f, 1.0f};

        console->input_baseline_offset = v2{5.0f, 2.0f + font->descent};

        Console_Cursor *cursor = &console->cursor;
        cursor->offset = console->input_baseline_offset;
        cursor->dim = v2{3.0f, font->ascent + font->descent + 1.0f};
        cursor->color1 = V4(0.2f, 0.2f, 0.2f, 1.0f);
        cursor->color2 = V4(1.0f, 1.0f, 1.0f, 1.0f);

        console->is_down = false;
    }
}

inline b32
command_equal(Console *console, char *str) {
    return string_equal(console->cbuf, console->cbuf_at, str, string_length(str));
}

internal void
draw_console(Console *console, Render_Group *group, f32 real_time, f32 width, f32 height) {
    if (console->dt != 0.0f) {
        f32 T = map01(console->dt, 0.0f, CONSOLE_TARGET_T);
        f32 next_y;

        next_y = lerp(width, T, height - console->half_dim.y);
        Rect2 console_rect = rect2_cen_half_dim(v2{width * 0.5f, next_y}, console->half_dim);
        v2 con_o = console_rect.min;
        push_rect(group, console_rect, 0.0f, console->bg_color);

        Console_Cursor *cursor = &console->cursor;
        Rect2 cursor_rect = rect2_min_dim(con_o + cursor->offset - v2{0.0f, console->font->descent}, cursor->dim);
        v4 cursor_color = lerp(cursor->color1, 0.5f*sin(real_time*5.0f) + 1.0f, cursor->color2);
        push_rect(group, cursor_rect, 0.0f, cursor_color);

        Rect2 r = string_rect(console->cbuf, V3(con_o + console->input_baseline_offset, 0.0f), console->font);
        cursor->offset = console->input_baseline_offset + v2{r.max.x - r.min.x};
    } else {
        // No need to draw.
    }
}

internal void
update_console(Console *console, Game_State *game_state, Input *input) 
{
    if (console->is_down) {
        if (toggled_down(input, KEY_HASHTILDE)) {
            console->is_down = false;
        }

        if (toggled_down_or_repeated(input, KEY_BACKSPACE) &&
            console->cbuf_at > 0) 
        {
            console->cbuf[--console->cbuf_at] = 0;
        }
        if (toggled_down_or_repeated(input, KEY_SPACE) &&
            console->cbuf_at < array_count(console->cbuf) - 1) 
        {
            console->cbuf[console->cbuf_at++] = ' ';
            console->cbuf[console->cbuf_at] = 0;
        }
        // @TODO: this will be one long if-else statements unless you make commands into a tree.
        if (toggled_down(input, KEY_ENTER)) {
#if 0
            if (command_equal(console, "freecam")) {
                if (game_state->mode == Game_Mode_Game) {
                    if (game_state->using_camera == game_state->free_camera) {
                        game_state->using_camera = game_state->player_camera;
                        input->focus = Input_Focus_Game;
                    }
                    else if (game_state->using_camera == game_state->player_camera) {
                        game_state->using_camera = game_state->free_camera;
                        input->focus = Input_Focus_Camera;
                    }
                    else {
                        INVALID_CODE_PATH;
                    }
                }
            }
#else
            if (0) {}
#endif
            else if (command_equal(console, "exit")) {
                input->quit_requested = true;
            }
            else {
                // @TODO: report unknown command via console.
            }
            console->cbuf[console->cbuf_at = 0] = 0;
        }

        for (u8 key = KEY_A; key <= KEY_Z; ++key) {
            if (console->cbuf_at >= array_count(console->cbuf) - 1)
                break;

            if (toggled_down_or_repeated(input, key)) {
                if (input->keys[KEY_LEFTSHIFT].is_down) {
                    console->cbuf[console->cbuf_at++] = 'A' + (key - KEY_A);
                    console->cbuf[console->cbuf_at] = 0;
                } 
                else {
                    console->cbuf[console->cbuf_at++] = 'a' + (key - KEY_A);
                    console->cbuf[console->cbuf_at] = 0;
                }
            }
        }

        for (u8 key = KEY_0; key <= KEY_9; ++key) {
            if (console->cbuf_at >= array_count(console->cbuf) - 1)
                break;

            if (toggled_down_or_repeated(input, key)) {
                console->cbuf[console->cbuf_at++] = '0' + (key - KEY_0);
                console->cbuf[console->cbuf_at] = 0;
            }
        }
    } 
    else { // console is up
        if (toggled_down(input, KEY_HASHTILDE)) {
            console->is_down = true;
        }
    }

    console->dt += (console->is_down ? 1.0f : -1.0f) * input->actual_dt;
    console->dt = clamp(console->dt, 0.0f, CONSOLE_TARGET_T);
}
