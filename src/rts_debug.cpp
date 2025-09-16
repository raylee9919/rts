/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



internal void
debug_update_game_speed(Debug_State *debug_state, Input *input)
{
    debug_state->speed_fade_t -= input->actual_dt;
    if (input->keys[KEY_LEFTCTRL].is_down) 
    {
        Debug_State *ds = debug_state;
        if (toggled_down(input, KEY_EQUAL) &&
            ds->current_speed_idx < array_count(ds->speed_slider) - 1) 
        {
            ++ds->current_speed_idx;
            ds->speed = ds->speed_slider[ds->current_speed_idx];
            ds->speed_fade_t = 1.0f;
        }

        if (toggled_down(input, KEY_MINUS) &&
            ds->current_speed_idx != 0) 
        {
            --ds->current_speed_idx;
            ds->speed = ds->speed_slider[ds->current_speed_idx];
            ds->speed_fade_t = 1.0f;
        }
    }
}

internal void
debug_draw_performance(Render_Group *render_group, Input *input, Asset_Font *font) 
{
    char buf[256];
    snprintf(buf, 256, "actual mspf: %.4f | fps: %d", 1000.0f*input->actual_dt, (s32)(1.0f/input->actual_dt + 0.5f));
    v3 base = v3{10, input->draw_dim.y - 30.0f, 0};
    string_op(String_Op_Draw, render_group, base + V3(2,-2,-1), buf, font, RGBA_BLACK);
    string_op(String_Op_Draw, render_group, base, buf, font);
}

internal void
debug_draw_game_speed_text(Debug_State *debug_state, Input *input, Render_Group *render_group, Asset_Font *font)
{
    if (debug_state->speed_fade_t > epsilon_f32) 
    {
        char *text = debug_state->speed_text[debug_state->current_speed_idx];
        v2 dim = get_dim(string_rect(text, {}, font));
        f32 alpha = lerp(0.0f, debug_state->speed_fade_t, 1.0f);
        f32 dy = 30*lerp(1.0f, debug_state->speed_fade_t, 0.0f);
        v2 cen = v2{0.5f * input->draw_dim.w, 0.7f * input->draw_dim.h - dy};
        cen += v2{10, -10};
        string_op(String_Op_Draw, render_group, V3(cen-0.5f*dim, -1), text, font, V4(V3(0.0f), alpha));
        cen -= v2{10, -10};
        string_op(String_Op_Draw, render_group, V3(cen-0.5f*dim, 0), text, font, V4(V3(1.0f), alpha));
    }
}
