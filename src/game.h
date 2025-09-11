/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright %s by Sung Woo Lee. All Rights Reserved. $
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

    Memory_Arena total_arena;

    Memory_Arena frame_arena;
    Temporary_Memory frame_temporary_memory;

    Memory_Arena asset_arena;
    Game_Assets *game_assets;

    Memory_Arena world_arena;
    World *world;

    Memory_Arena debug_arena;
    Debug_State *debug_state;

    Memory_Arena ui_arena;

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

    Platform_Work_Queue *high_priority_queue;
    Platform_Work_Queue *low_priority_queue;
    Work_Memory_Arena work_arenas[4];

    Navmesh navmesh;
};

struct Load_Asset_Work_Data 
{
    Game_Assets         *game_assets;
    Memory_Arena        *assetArena;
    Asset_ID            assetID;
    const char          *fileName;
    Work_Memory_Arena   *workSlot;
};
