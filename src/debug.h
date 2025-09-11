/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2024 by Sung Woo Lee. All Rights Reserved. $
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
