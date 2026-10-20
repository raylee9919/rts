// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GAME_H
#define RTS_GAME_H

#include "basic/core.h"
#include "os/os.h"
#include "math/math.h"
#include "handle.h"
#include "terrain.h"
#include "camera.h"
#include "input.h"

#define MAX_ENTITIES        16384
#define WORLD_UP            vec3{ 0.f,  0.f,  1.f}
#define FORWARD_VECTOR      vec4{ 0.f,  1.f,  0.f, 1.f}
#define RIGHT_VECTOR        vec4{ 1.f,  0.f,  0.f, 1.f}
#define UP_VECTOR           vec4{ 0.f,  0.f,  1.f, 1.f}
#define NEAR_Z              0.1f
#define FAR_Z               3000.0f

// Be meticulous whenever you add or remove a field from the entitiy struct!
struct Entity 
{
  u64             generational_id;

  Handle          parent;
  Handle          first; // first child
  Handle          last;  // last  child
  Handle          next;  // next sibling
  Handle          prev;  // prev sibling

  vec3            position;
  vec3            scale;

  Guid            mesh;

  Guid            animation_id; // @Temporary
};

struct Game_Storage 
{
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
struct Game_State 
{
  Game_Storage    storage;

  f64             time;

  u64             next_generational_id = 1;
  u64             num_entities;
  Handle          root;

  // Singly-linked entity free list
  Handle          first_free_entity;
  Handle          last_free_entity;

  Input_State     inputs;

  Camera          camera;

  Terrain         terrain;
};

extern Game_State *game_state;


void       
game_state_init(Game_State **game_state_pptr);

void       
game_state_deinit(Game_State *g);


void       
game_init(f64 time_init);

void       
game_shutdown();


void       
game_copy(Game_State *dst, Game_State *src);

void *
game_alloc(Game_State *g, u64 size, u64 alignment);


Entity *
entity_alloc(Game_State *g);

void       
entity_dealloc(Game_State *g, Entity *entity);

void       
entity_dealloc(Game_State *g, Handle handle);

Entity *
entity_from_handle(Game_State *g, Handle handle);

Handle 
handle_from_entity(Game_State *g, Entity *entity);

void 
entity_add_child(Game_State *g, Handle parent, Handle child);

void 
entity_remove_child(Game_State *g, Handle parent, Handle child);

void 
entity_dfs(Game_State *g, Handle root, void *user_data, void (*proc)(Game_State *g, Entity *entity, u64 index, void *user_data));


Handle 
handle_from_pointer(Game_State *g, void *ptr);

u8 *
pointer_from_handle(Game_State *g, Handle handle);


#endif // RTS_GAME_H
