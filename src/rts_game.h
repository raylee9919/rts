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
struct Ui;
struct Debug_State;
struct Entity;

#define MAX_ENTITY_COUNT 1024

struct World 
{
    Entity *entities[MAX_ENTITY_COUNT];
    u32 entity_count;

    Camera *cameras[256];
    u32 camera_count;

    u32 next_entity_id;
};

struct Game_Assets 
{
    Bitmap debug_bitmap;

    Asset_Font debug_font;
    Asset_Font menu_font;
    Asset_Font console_font;
    Asset_Font karmina;
    Asset_Font times;

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
    Game_Mode_Editor,
    Game_Mode_Game,
    Game_Mode_Menu,
};

struct Game_State 
{
    b32 initted;

    b32 editor_initted;

    Arena *frame_arena;

    Arena *asset_arena;
    Game_Assets *game_assets;

    Arena *world_arena;
    World *world;

    Arena *debug_arena;
    Debug_State *debug_state;

    Arena *ui_arena;

    f32 real_time;
    f32 game_time;

    Game_Mode mode;

    Random_Series random_series;

    Camera *controlling_camera;
    m4x4 view_proj; // @Ain't thrilled about it. -Ray
    Camera *game_camera;
    Camera *debug_camera;
    Camera *orthographic_camera;

    u32 active_entity_id;

    Navmesh navmesh;
};

#endif // RTS_GAME_H
