// Copyright Seong Woo Lee. All Rights Reserved.

#include "game.h"
#include "basic/log.h"
#include "os/os.h"
#include "profiler/profiler.h"

Game_State *game_state;

void 
game_state_init(Game_State **game_state_pptr) 
{
  // Reserve gigantic memory space and allocate game state in there.
  u64 page_size = os_query_page_size();
  u64 rsv = align_up(Gigabytes(128), page_size);
  u8 *ptr = (u8*)os_reserve(rsv);

  // First commit
  u64 sz = sizeof(Game_State);
  u64 cmt = align_up(sz, page_size);
  R_ASSERT( os_commit(ptr, cmt) );

  Game_State *g = (Game_State *)ptr;
  Construct(g);

  g->storage.base      = ptr;
  g->storage.reserved  = rsv;
  g->storage.committed = cmt;
  g->storage.used     += sz;
  g->storage.page_size = page_size;

  *game_state_pptr = g;
}

void 
game_state_deinit(Game_State *g) 
{
  os_release(g->storage.base, g->storage.reserved);
}

void 
game_init() 
{
  // Initialize game state
  game_state_init(&game_state);

  // Allocate root entity
  Entity *root = entity_alloc(game_state);
  game_state->root = handle_from_entity(game_state, root);

  // Init terrain
  terrain_init(game_state,
               &game_state->terrain,
               NULL_GUID, 12.f, 1.f);

  // Init navmesh
  navmesh_init(&game_state->navmesh);

  // Init aabb tree
  game_state->arena = arena_alloc();
  aabb_tree_init(&game_state->aabb_tree, game_state->arena);

  // Init camera
  camera_init(game_state, &game_state->camera, vec3(0.f, 0.f, 0.f), 0.f, 32.f);

  log_info(S("Initialized game state."));
}

void 
game_shutdown() 
{
  Game_State *g = game_state;

  navmesh_deinit(&g->navmesh);
  os_release(g->storage.base, g->storage.reserved);

  log_info(S("Shutdown game states."));
}

void 
game_copy(Game_State *dst, Game_State *src) 
{
  ProfileScope;

  if (dst->storage.used < src->storage.used) {
    game_alloc(dst, src->storage.used - dst->storage.used, 1);
  }

  Game_Storage tmp = dst->storage;

  memcpy(dst->storage.base, src->storage.base, src->storage.used);
  dst->storage = tmp;
  dst->storage.used = src->storage.used;
}

void *
game_alloc(Game_State *g, u64 size, u64 alignment) 
{
  Game_Storage *s = &g->storage;

  u8 *ptr = (u8*)align_up((void*)(s->base + s->used), alignment);
  u8 *opl = ptr + size;
  s->used = opl - s->base;

  if (s->used > s->committed) {
    u64 commit_size = align_up(s->used - s->committed, s->page_size);
    R_ASSERT( os_commit(s->base + s->committed, commit_size) );
    s->committed += commit_size;
  }

  return ptr;
}

Entity *
entity_alloc(Game_State *g) 
{
  Entity *entity = entity_from_handle(g, g->first_free_entity);

  if (entity) {
    if (g->first_free_entity == g->last_free_entity) {
      g->first_free_entity = {};
      g->last_free_entity  = {};
    } else {
      g->first_free_entity = entity->next;
    }
  } else {
    entity = (Entity *)game_alloc(g, sizeof(Entity), align_of(Entity));
  }

  R_ASSERT(entity);

  memset(entity, 0, sizeof(Entity));

  entity->scale = vec3(1.f);

  // Assign generational ID
  entity->generational_id = g->next_generational_id++;

  // Attach to root
  entity_add_child(g, g->root, handle_from_entity(g, entity));

  g->num_entities += 1;

  return entity;
}

void 
entity_dealloc(Game_State *g, Entity *entity) 
{
  // @Todo: Must remove from the tree, decommit page?
  entity->generational_id = 0;
  g->num_entities -= 1;
}

void 
entity_dealloc(Game_State *g, Handle handle) 
{
  Entity *entity = entity_from_handle(g, handle);
  if (entity) {
    entity_dealloc(g, entity);
  }
}

Handle 
handle_from_pointer(Game_State *g, void *ptr) 
{
  Handle h = {};
  h.offset = (u8*)ptr - g->storage.base;
  return h;
}

u8 *
pointer_from_handle(Game_State *g, Handle handle) 
{
  if (handle.is_null()) return NULL;
  u8 *ptr = (u8 *)(g->storage.base + handle.offset);
  return ptr;
}

Handle 
handle_from_entity(Game_State *g, Entity *entity) 
{
  Handle h;
  h.offset = (u8*)entity - g->storage.base;
  h.generational_id = entity->generational_id;
  return h;
}

Entity *
entity_from_handle(Game_State *g, Handle handle) 
{
  if (handle.is_null()) return NULL;
  Entity *entity = (Entity *)(g->storage.base + handle.offset);
  if (entity->generational_id == handle.generational_id) {
    return entity;
  }
  return NULL;
}

void entity_add_child(Game_State *g, Handle parent, Handle child) {
    Entity *pa = entity_from_handle(g, parent);
    Entity *ch = entity_from_handle(g, child);

    if (!pa || !ch)  return;

    ch->parent = parent;

    if (pa->first.is_null() && pa->last.is_null()) {
        pa->first = child;
        pa->last  = child;
        ch->prev  = child;
        ch->next  = child;
    } else {
        Entity *first = entity_from_handle(g, pa->first);
        Entity *last  = entity_from_handle(g, pa->last);

        R_ASSERT(first && last); // Deallocated entity must have been removed

        first->prev = child;
        last->next  = child;
        ch->prev    = pa->last;
        ch->next    = pa->first;
        pa->last    = child;
    }
}

void entity_remove_child(Game_State *g, Handle parent, Handle child) {
    // @Todo
}

static u64 
_entity_dfs_internal(Game_State *g, 
                     Handle      root, 
                     u64         index, 
                     void       *user_data, 
                     void        (*proc)(Game_State *g, Entity *entity, u64 index, void *user_data) )
{
  Entity *entity = entity_from_handle(g, root);
  if (!entity)  return index;

  // Let's skip the dummy root entity.
  if (root != g->root) {
    proc(g, entity, index, user_data);
    index += 1;
  }

  Handle first = entity->first;
  if (first.is_null())  return index;

  Handle child = first;

  for (;;) {
    index = _entity_dfs_internal(g, child, index, user_data, proc);

    Entity *child_entity = entity_from_handle(g, child);

    R_ASSERT(child_entity);  // Deallocated entity must have been removed

    child = child_entity->next;
    if (child == first)  break;
  }

  return index;
}

void entity_dfs(Game_State *g, 
                Handle      root, 
                void       *user_data, 
                void        (*proc)(Game_State *g, Entity *entity, u64 index, void *user_data)) 
{
  _entity_dfs_internal(g, root, 0, user_data, proc);
}

//
// Selection
//
void
selection_clear(Game_State* g, 
                s32         player)
{
  R_ASSERT(player < MAX_PLAYERS);

  for (s32 i = 0; i < g->num_selected[player]; ++i)
  {
    Entity *e = entity_from_handle(g, g->selected[player][i]);
    if (e)
    {
      e->selected = false;
    }
  }

  g->num_selected[player] = 0;
}

void
selection_sanitize(Game_State* g, 
                   s32         player)
{
  for (s32 i = 0; i < g->num_selected[player]; )
  {
    Entity *e = entity_from_handle(g, g->selected[player][i]);
    if (!e)
    {
      g->num_selected[player] -= 1;
      g->selected[player][i] = g->selected[player][g->num_selected[player]];
    }
    else
    {
      i += 1;
    }
  }
}

void 
selection_append(Game_State* g, 
                 s32         player, 
                 Handle      handle)
{
  R_ASSERT(player < MAX_PLAYERS);
  if (g->num_selected[player] >= MAX_SELECTED_ENTITES)
  {
    return;
  }

  Entity *e = entity_from_handle(g, handle);
  if (e)
  {
    e->selected = true;

    g->selected[player][g->num_selected[player]] = handle;
    g->num_selected[player] += 1;
  }
}
