// Copyright Seong Woo Lee. All Rights Reserved.

#include "game.h"
#include "basic/log.h"
#include "os/os.h"
#include "profiler/profiler.h"
#include "shared.h"
#include "renderer/renderer.h"
#include "renderer/immediate.h"

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
selection_clear(Game_State  *g, 
                Player_Index player)
{
  R_ASSERT(player < PLAYER_MAX);

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
selection_sanitize(Game_State  *g, 
                   Player_Index player)
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
selection_append(Game_State  *g, 
                 Player_Index player,
                 Handle       handle)
{
  R_ASSERT(player < PLAYER_MAX);
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

void 
game_input_process(Game_State *g) 
{
  /*
    Entity selection
      On click,    only the first hit entity must be selected from the ray intersection test.
      On drag end, all the entities overlapping with the view frustum must be selected.
  */
  if (key_transitioned_down(&g->inputs, MOUSE_BUTTON_LEFT))
  {
    auto [x,y,success] = get_mouse_pointer_position(shared->window);
    if (success)
    {
      g->game_input.mouse_clicked_postion = playfield_from_window(x, y);
    }
  }

  if (key_transitioned_up(&g->inputs, MOUSE_BUTTON_LEFT))
  {
    auto [x,y,success] = get_mouse_pointer_position(shared->window);
    if (success)
    {
      vec2 p1 = g->game_input.mouse_clicked_postion;
      vec2 p2 = playfield_from_window(x, y);

      if (distance(p1, p2) > 2.f)
      {
        // @Todo: Dragged. Better rule other than distance?
        // Also, make some room in dx and dy?

        // To NDC
        p1 = p1 * vec2(1.f/RESOLUTION_X, 1.f/RESOLUTION_Y) * vec2(2.f, -2.f) + vec2(-1.f, 1.f);
        p2 = p2 * vec2(1.f/RESOLUTION_X, 1.f/RESOLUTION_Y) * vec2(2.f, -2.f) + vec2(-1.f, 1.f);

        vec4 v1 = g->camera.inv_proj * vec4(p1, 0.f, 1.f);
        vec4 v2 = g->camera.inv_proj * vec4(p2, 0.f, 1.f);

        v1.xyz /= v1.w;
        v2.xyz /= v2.w;

        // Make view-projection matrix from the frustum.
        f32 xmin = min(v1.x, v2.x);
        f32 xmax = max(v1.x, v2.x);
        f32 ymin = min(v1.y, v2.y);
        f32 ymax = max(v1.y, v2.y);
        m4x4 view = g->camera.view;
        m4x4 proj = persp_asymmetric_rh(xmin, xmax, ymin, ymax, NEAR_Z, FAR_Z);
        m4x4 view_proj = proj * view;

        Array<Handle> broadphase_result = aabb_tree_query(&g->aabb_tree, &view_proj, tctx.temp);
        Array<Handle> result(tctx.temp);

        for (u32 i = 0; i < broadphase_result.count; ++i)
        {
          Handle handle = broadphase_result[i];
          Entity *e = entity_from_handle(g, handle);
          if (e)
          {
            AABB box = aabb_from_aabbv(aabbv_translate(e->tight_box, e->position));
            if (view_frustum_aabb_intersect(&view_proj, &box))
            {
              append(&result, handle);
            }
          }
        }

        if (result.count > 0)
        {
          selection_clear(g, PLAYER_0);
        }

        for (u32 i = 0; i < result.count; ++i)
        {
          Handle handle = result.data[i];
          selection_append(g, PLAYER_0, handle);
        }
      }
      else
      {
        // Clicked
        p1 = p1 * vec2(1.f/RESOLUTION_X, 1.f/RESOLUTION_Y) * vec2(2.f, -2.f) + vec2(-1.f, 1.f);
        vec4 H = g->camera.inv_view_proj * vec4(p1, 0.f, 1.f);
        vec3 P = H.xyz / H.w;
        vec3 O = g->camera.position;
        vec3 D = normalize(P - O);

        Ray ray = make_ray(O, D);

        Array<Handle> handles = aabb_tree_query(&g->aabb_tree, &ray, tctx.temp);
        if (handles.count > 0)
        {
          f32 tmin = INFINITY;
          Handle handle = {};

          for (u32 i = 0; i < handles.count; ++i)
          {
            Entity *e = entity_from_handle(g, handles[i]);
            if (e)
            {
              AABB box = aabb_from_aabbv(aabbv_translate(e->tight_box, e->position));
              f32 t = INFINITY;
              if (ray_aabb_intersect(&ray, &t, &box))
              {
                if (tmin > t)
                {
                  tmin = t;
                  handle = handles[i];
                }
              }
            }
          }

          if (!handle.is_null())
          {
            selection_clear(g, PLAYER_0);
            selection_append(g, PLAYER_0, handle);
          }
        }
      }
    }
  }

  if (key_down(&g->inputs, MOUSE_BUTTON_LEFT))
  {
    auto [x,y,success] = get_mouse_pointer_position(shared->window);
    if (success)
    {
      vec4 color    = vec4(0.3f, 1.f, 0.4f, 0.9f);
      f32 thickness = 1.5f;
      vec2 _00 = g->game_input.mouse_clicked_postion;
      vec2 _11 = playfield_from_window(x, y);
      vec2 _01 = vec2(_11.x, _00.y);
      vec2 _10 = vec2(_00.x, _11.y);
      draw_line(_00, _01, color, thickness);
      draw_line(_01, _11, color, thickness);
      draw_line(_11, _10, color, thickness);
      draw_line(_10, _00, color, thickness);
    }
  }
}
