// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once



struct Entity;

#define MAX_ENTITY_COUNT 1024

struct Game_Assets {
    Arena* arena;

    Bitmap debug_bitmap;

    Model* skeleton_model;
    Model* warrior_model;
    Model* castle_model;
    Model* sword_model;
    Model* crate_model;
    Model* sphere_model;
    Model* plane_model;
    Model* rock_model;

    Mesh   skybox_mesh;
    Bitmap skybox_textures[6];

    Animation* xbot_idle;
    Animation* xbot_run;
    Animation* xbot_die;
    Animation* xbot_attack;
               
    Animation* warrior_idle;
};

struct Chunk {
    Entity* first_entity;
    Entity* last_entity;
};

struct Navmesh {
    Cdt_Context   ctx;
    cdt_triangle *triangles;
    int           triangle_count;
};

struct Game_State {
    b32 initted;

    b32 editor_initted;

    Arena* frame_arena;

    Game_Assets* assets;

    u32 draw_width;
    u32 draw_height;

    u32 window_width;
    u32 window_height;

    Random_Series random_series;

    Arena*      map_arena;
    v2          chunk_size;
    u16         chunk_count_x;
    u16         chunk_count_y;
    v2          map_size;
    Chunk*      chunks;
    Navmesh     navmesh;

    Arena*      entity_arena;
    Entity*     first_free_entity;
    Entity*     last_free_entity;
    u64         next_generational_id; // 0 is null.
    Entity*     root_entity;
    Entity*     entity_table;
    u32         entity_table_size;

    u64         game_camera_id;
    u64         debug_camera_id;
    u64         controlling_camera_id;

    List <u64>  selected_entities;

    f32         max_radius;
};
