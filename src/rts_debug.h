#ifndef RTS_DEBUG_H
#define RTS_DEBGUG_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2024 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Debug_State
{
    b32         initted;

    f32         speed;
    f32         speed_fade_t;
    u32         current_speed_idx;
    f32         speed_slider[5];
    char        *speed_text[5];

    Console console;
};


internal void debug_update_game_speed(Debug_State *debug_state, Input *input);
internal void debug_draw_performance(Render_Group *render_group, Input *input, Asset_Font *font);
internal void debug_draw_game_speed_text(Debug_State *debug_state, Input *input, Render_Group *render_group, Asset_Font *font);



#endif //RTS_DEBUG_H
