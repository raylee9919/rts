// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GAME_H
#define RTS_GAME_H

#include "basic/core.h"
#include "os/os.h"
#include "math/math.h"

#define MAX_ENTITIES        16384
#define WORLD_UP            vec3{ 0.f,  1.f,  0.f}
#define FORWARD_VECTOR      vec4{ 0.f,  0.f, -1.f, 1.f}
#define RIGHT_VECTOR        vec4{ 1.f,  0.f,  0.f, 1.f}
#define UP_VECTOR           vec4{ 0.f,  1.f,  0.f, 1.f}
#define NEAR_Z              1e-3f
#define FAR_Z               1e9f


// You can think of Handle as a pointer. The difference is that while a
// "real" pointers is in the process's address space, a Handle is an offset
// into the Game_Storage's HUGE reserved address space.
//
struct Handle {
    u64             offset;
    u64             generational_id;

    force_inline bool operator == (Handle &other) { return ((offset == other.offset) && (generational_id == other.generational_id)); }
    force_inline bool operator != (Handle &other) { return ((offset != other.offset) || (generational_id != other.generational_id)); }
    force_inline bool is_null() { return ((offset == 0) && (generational_id == 0)); }
};

// Be meticulous whenever you add or remove a field from the entitiy struct!
struct Entity {
    u64             generational_id;

    Handle          parent;
    Handle          first; // first child
    Handle          last;  // last  child
    Handle          next;  // next sibling
    Handle          prev;  // prev sibling

    vec3            position;
    vec3            scale;

    Guid            mesh;
    Guid            material;

    u64             animation_player; // Storage offset, 0 if none. May be shared.
};

struct Camera {
    vec3            position;
    f32             yaw;
    f32             pitch;
};

struct Game_Storage {
    u8             *base;
    u64             reserved;
    u64             committed;
    u64             used;
    u64             page_size;
};

// Make sure you're not adding things that make state memcpy difficult. Entity
// is represented as an offset in the storage. So it's kind of a pointer, but
// memcpyable.
//
struct Game_State {
    Game_Storage    storage;

    f64             time;

    u64             next_generational_id = 1;
    u64             num_entities;
    Handle          root;

    // Singly-linked entity free list
    Handle          first_free_entity;
    Handle          last_free_entity;

    Camera          camera;
};


extern Game_State *game_state;


void       game_state_init(Game_State **game_state_pptr);
void       game_state_deinit(Game_State *g);
void       game_init(f64 time_init);
void       game_shutdown();
void       game_copy(Game_State *dst, Game_State *src);
void      *game_alloc(Game_State *g, u64 size, u64 alignment);

Entity    *entity_alloc(Game_State *g);
void       entity_dealloc(Game_State *g, Entity *entity);
void       entity_dealloc(Game_State *g, Handle handle);

Entity    *entity_from_handle(Game_State *g, Handle handle);
Handle     handle_from_entity(Game_State *g, Entity *entity);

void       entity_add_child(Game_State *g, Handle parent, Handle child);
void       entity_remove_child(Game_State *g, Handle parent, Handle child);

void       entity_dfs(Game_State *g, Handle root, void *user_data, void (*proc)(Game_State *g, Entity *entity, u64 index, void *user_data));


#endif // RTS_GAME_H
