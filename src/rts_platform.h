#ifndef RTS_PLATFORM_H
#define RTS_PLATFORM_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Renderer;

struct Platform
{
    Arena      *arena;

    OS         *os;
    void       *game_state;
    Utf8        data_path;
    f32         dt;

    Renderer    *renderer;

    u32         draw_width;
    u32         draw_height;

    u32         window_width;
    u32         window_height;

    b32         exit_requested;
};

#define GAME_UPDATE_AND_RENDER(name) void name(struct Platform *platform,\
                                               struct Render_Commands *render_commands)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);


#endif //RTS_PLATFORM_H
