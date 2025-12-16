#ifndef RTS_GAME_H
#define RTS_GAME_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

struct Entity;

#define MAX_ENTITY_COUNT 1024

struct Game_Assets {
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

struct Chunk {
    Entity *first_entity;
    Entity *last_entity;
};

struct Navmesh {
    Cdt_Context   ctx;
    cdt_triangle *triangles;
    int           triangle_count;
};

struct Game_State {
    b32 initted;

    b32 editor_initted;

    Arena *frame_arena;

    Game_Assets *assets;

    u32 draw_width;
    u32 draw_height;

    u32 window_width;
    u32 window_height;

    Random_Series random_series;

    Arena      *map_arena;
    v2u         chunk_size;
    u32         chunk_count_x;
    u32         chunk_count_y;
    v2          map_size;
    Chunk      *chunks;
    Navmesh     navmesh;

    Arena      *entity_arena;
    Entity     *first_free_entity;
    Entity     *last_free_entity;
    Entity     *entity_table;
    u32         entity_table_size;
    u32         next_generational_id;

    u32         game_camera_id;
    u32         debug_camera_id;
    u32         controlling_camera_id;
};

#endif // RTS_GAME_H
