#ifndef RTS_GAME_H
#define RTS_GAME_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Camera;
struct Entity;

#define MAX_ENTITY_COUNT 1024

struct World 
{
    Arena *arena;

    Entity *entities[MAX_ENTITY_COUNT];
    u32 entity_count;

    Camera *cameras[256];
    u32 camera_count;

    u32 next_entity_id;
};

struct Game_Assets 
{
    Arena *arena;

    Bitmap debug_bitmap;

    Model *xbot_model;
    Model *crate_model;

    Mesh skybox_mesh;
    Bitmap skybox_textures[6];

    Model *sphere_model;
    Model *plane_model;

    Model *rock_model;

    Animation *xbot_idle;
    Animation *xbot_run;
    Animation *xbot_die;
    Animation *xbot_attack;
};

enum Game_Mode 
{
    GAME_MODE_EDITOR,
    GAME_MODE_GAME,
};

struct Game_State 
{
    b32 initted;

    b32 editor_initted;

    Arena *frame_arena;

    Game_Assets *assets;

    World *world;

    u32 draw_width;
    u32 draw_height;

    u32 window_width;
    u32 window_height;

    f32 dt_real;
    f32 dt_game;
    f32 time;

    Game_Mode mode;

    Random_Series random_series;

    Camera *controlling_camera;
    m4x4 view_proj; // @Todo: bad :<
    Camera *game_camera;
    Camera *debug_camera;
    Camera *orthographic_camera;

    u32 active_entity_id;

    Navmesh *navmesh;
};

#endif // RTS_GAME_H
