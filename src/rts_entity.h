#if !defined(RTS_ENTITY_H)
#define RTS_ENTITY_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

enum Entity_Type {
    ENTITY_TYPE_INVALID = 0,
    ENTITY_TYPE_CAMERA,
    ENTITY_TYPE_SOLDIER,
};

typedef u16 Entity_Command;
enum {
    ENTITY_CMD_IDLE = 0,
    ENTITY_CMD_MOVE,
    ENTITY_CMD_DIEING,
};

typedef u64 Entity_Flags;
enum {
    ENTITY_FLAG_DEAD              = 0x1,
    ENTITY_FLAG_CHUNK_PARTITIONED = 0x2,
    ENTITY_FLAG_COLLIDEABLE       = 0x4,
};

struct Entity {
    u32          id; // generational id
    Entity_Type  type;
    Entity_Flags flags;

    Entity_Command command;

    Entity     *first;
    Entity     *last;
    Entity     *next;
    Entity     *prev;
    Entity     *next_in_chunk;
    Entity     *prev_in_chunk;

    v3          position;
    Quaternion  orientation;
    v3          scaling;

    f32         radius;
    u16         chunk_x;
    u16         chunk_y;

    v3          velocity;
    v3          accel;
    v3          destination;
    f32         speed;
    
    f32         transition_t;

    Animation_Channel   animation_channels[1];
    m4x4                *animation_transform;
    Animation           *idle_animation;
    Animation           *running_animation;
    Animation           *die_animation;
    Animation           *attack_animation;

    Model *model;

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
};









#endif // RTS_ENTITY_H
