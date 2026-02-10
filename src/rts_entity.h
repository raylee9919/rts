// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

enum Entity_Type {
    ENTITY_TYPE_INVALID = 0,
    ENTITY_TYPE_ROOT,

    ENTITY_TYPE_CAMERA,
    ENTITY_TYPE_SOLDIER,
    ENTITY_TYPE_SWORD,
    ENTITY_TYPE_CASTLE,
};

typedef u16 Entity_Command;
enum {
    ENTITY_CMD_STOP = 0,
    ENTITY_CMD_MOVE,
    ENTITY_CMD_ATTACK,
    ENTITY_CMD_DIEING,
};

typedef u64 Entity_Flags;
enum {
    ENTITY_FLAG_DEAD              = (1<<0),

    ENTITY_FLAG_IS_UNIT           = (1<<1),
    ENTITY_FLAG_CHUNK_PARTITIONED = (1<<2),
    ENTITY_FLAG_COLLIDEABLE       = (1<<3),
    ENTITY_FLAG_SELECTED          = (1<<4),

    ENTITY_FLAG_GAME_CAMERA       = (1<<5),
    ENTITY_FLAG_FREE_CAMERA       = (1<<7),

    ENTITY_FLAG_SHOWS_ON_MINIMAP  = (1<<8),
};

typedef u8 Team;
enum {
    TEAM_PLAYER = 0,
    TEAM_ENEMY  = 1
};

struct Entity {
    u64          id; // generational id
    Entity_Type  type;
    Entity_Flags flags;


    Entity_Command command;


    // Table entries or children in scene hierarchy.
    Entity*     first;
    Entity*     last;

    // Free list or table which are mutually exclusive!
    Entity*     next_in_table;
    Entity*     prev_in_table;

    // Scene hierarchy siblings
    Entity*     next_sibling;
    Entity*     prev_sibling;

    // Chunk
    Entity*     next_in_chunk;
    Entity*     prev_in_chunk;


    v3          position;
    Quaternion  orientation;
    v3          scaling;


    // @Todo: Unsafe
    Entity*     parent;
    Joint_Id    parent_joint_id;
    v3          local_position;
    Quaternion  local_orientation;


    f32         radius;
    u16         chunk_x;
    u16         chunk_y;

    v3          velocity;
    v3          destination;
    f32         speed;
    
    f32         transition_t;

    u64         target_id;


    f32         find_target_t;
    f32         find_target_max_t;


    Model* model;

    Animation_Channel    animation_channels[1];
    m4x4*                animation_transform;
    Animation*           idle_animation;
    Animation*           running_animation;
    Animation*           die_animation;
    Animation*           attack_animation;

    
    Team team;


    // Movement curve's horizontal axis is time.
    //                    vertical axis is speed.
    f32 speed_t;
    f32 min_t;
    f32 max_t;

    // Attack curve's horizontal axis is time.
    //                  vertical axis is normalized 0 to 1.
    f32 attack_t;
    f32 prev_attack_t;
    f32 attack_max_t;


    f32 hitpoints;
    

    f32 focal_length;
    f32 width;
    f32 height;
    f32 N;
    f32 F;

    m4x4 V;
    m4x4 P;
    m4x4 VP;

    Queue<v3> waypoint_queue;

    // @Temporary:
    Array<v2> l_points;
    Array<v2> r_points;
    Queue<v3> debug_waypoint_queue;

    // Navmesh
    f32 navmesh_scale;
};






internal Chunk* chunk_from_chunk_position(u16 x, u16 y);
internal void chunk_position_from_world_position(f32 world_x, f32 world_y, u16* out_x, u16* out_y);
