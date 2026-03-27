// Copyright Seong Woo Lee. All Rights Reserved.

struct Platform
{
    Arena*      arena;

    OS*         os;
    Os_Handle   window_handle;
    void*       game_state;
    Utf8        data_path;
    f32         dt;

    struct Renderer* renderer;

    u32         draw_width;
    u32         draw_height;

    u32         window_width;
    u32         window_height;

    b32         exit_requested;
};

#define GAME_UPDATE_AND_RENDER(name) void name(struct Platform *platform,\
                                               struct Render_Commands *render_commands)
typedef GAME_UPDATE_AND_RENDER(Game_Update_And_Render);
