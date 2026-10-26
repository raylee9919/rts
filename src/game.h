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
#include "navigation.h"
#include "aabb_tree.h"

#define MAX_ENTITIES        16384
#define WORLD_UP            vec3{ 0.f,  0.f,  1.f}
#define FORWARD_VECTOR      vec4{ 0.f,  1.f,  0.f, 1.f}
#define RIGHT_VECTOR        vec4{ 1.f,  0.f,  0.f, 1.f}
#define UP_VECTOR           vec4{ 0.f,  0.f,  1.f, 1.f}
#define NEAR_Z              0.1f
#define FAR_Z               3000.0f

#define MAX_WAYPOINTS          16
#define MAX_PLAYERS             4
#define MAX_SELECTED_ENTITES   12


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

  // Movement
  f32             radius;
  vec3            goal;
  vec3            waypoints[MAX_WAYPOINTS];

  // 'tight_aabb' is in model (local? whatever) space.
  // 'loose_aabb_world' is in world space coordinates, because 
  // we must check if the tight box goes out of the loose one.
  AABBV           tight_box;
  AABBV           loose_box_local;
  AABBV           loose_box_world;

  // Index into the proxy in the AABB tree.
  s32             aabb_tree_proxy;

  // Whether I am selected or not
  b32             selected;
};

struct Game_Storage 
{
  u8             *base;
  u64             reserved;
  u64             committed;
  u64             used;
  u64             page_size;
};

struct Game_Input
{
  vec2  mouse_clicked_postion;
};

// Make sure you're not adding things that make state memcpy difficult. Entity
// is represented as an offset in the storage. So it's kind of a pointer, but
// memcpyable.
//
// It makes my life difficult with this storage and handle thing.
// I need pointers in my hand as well. If something involved pointer, 
// build it again at game load or something.
//                                                     2026-10-23 swl
//
struct Game_State 
{
  Game_Storage    storage;

  u64             tick; // first tick = 1
  f64             time;

  u64             next_generational_id = 1;
  u64             num_entities;
  Handle          root;

  // Singly-linked entity free list
  Handle          first_free_entity;
  Handle          last_free_entity;


  Input_State     inputs;
  Game_Input      game_input;

  Camera          camera;

  Terrain         terrain;

  Navmesh         navmesh;

  Arena          *arena; // My life is hard. Ditch storage or do something about it.
  Box_Tree        aabb_tree;

  // Selected entities by each player
  Handle          selected[MAX_PLAYERS][MAX_SELECTED_ENTITES];
  s32             num_selected[MAX_PLAYERS];
};

extern Game_State *game_state;


void       
game_state_init(Game_State **game_state_pptr);

void       
game_state_deinit(Game_State *g);


void       
game_init();

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


// 
// Selection
//
void
selection_clear(Game_State* g, 
                s32         player);

void
selection_sanitize(Game_State* g, 
                   s32         player);
void 
selection_append(Game_State* g, 
                 s32         player, 
                 Handle      handle);



#endif // RTS_GAME_H
