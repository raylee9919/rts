// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GAME_H
#define RTS_GAME_H


#define MAX_ENTITIES        16384
#define WORLD_UP            v3{ 0.f,  1.f,  0.f}
#define FORWARD_VECTOR      v4{ 0.f,  0.f, -1.f, 1.f}
#define RIGHT_VECTOR        v4{ 1.f,  0.f,  0.f, 1.f}
#define UP_VECTOR           v4{ 0.f,  1.f,  0.f, 1.f}
#define NEAR_Z              1e-3f
#define FAR_Z               1e9f


struct Entity {
    u64             generational_id;

    v3              position;

    Guid            mesh;
    Guid            material;
};

struct Camera {
    v3              position;
    f32             yaw;
    f32             pitch;
};

struct Game_Storage {
    u8              *base;
    u64             reserved;
    u64             used;
};

// Make sure you're not adding things that make state memcpy difficult. Entity
// is represented as an offset in the storage. So it's kind of a pointer, but
// memcpyable.
//
struct Game_State {
    Game_Storage    storage;

    f64             time;

    u64             next_generational_id = 1;
    u64             entities[MAX_ENTITIES];

    Camera          camera;
};

global Game_State   *game_state;


internal void       game_state_init(Game_State **game_state_pptr);
internal void       game_init(f64 time_init);
internal void       game_shutdown();
internal void       game_copy(Game_State *dst, Game_State *src);
internal Entity     *entity_alloc(Game_State *g);
internal void       entity_dealloc(Game_State *g);
internal Entity     *entity_from_offset(Game_State *g, u64 offset);


#endif // RTS_GAME_H
