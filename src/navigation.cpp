// Copyright Seong Woo Lee. All Rights Reserved.

#include "./navigation.h"
#include "basic/log.h"
#include "profiler/profiler.h"
#include "math/cdt.h"
#include "game.h"

struct Wall {
  vec2 a, b;
};

static Game_State *
game_state_from_navmesh(Navmesh* navmesh)
{
  return (Game_State*)((u8*)navmesh - navmesh->offset_in_game_state);
}

static Terrain *
terrain_from_navmesh(Navmesh *navmesh)
{
  return &game_state_from_navmesh(navmesh)->terrain;
}

static Array<Wall>
collect_walls(Navmesh *navmesh)
{
  Array<Wall> result = {};
  result.allocator = tctx.temp;

  Terrain *t  = terrain_from_navmesh(navmesh);
  s64 cw = t->cells_x;
  s64 ch = t->cells_y;

  // As CDT needs walls that only touch at their endpoints, 
  // checkerboard pattern is checked so runs are cut at that vertex.
  auto checker = [&](s64 x, s64 y) -> b32 {
    b32 a = terrain_passable(t, x - 1, y - 1), b = terrain_passable(t, x, y - 1);
    b32 c = terrain_passable(t, x - 1, y),     d = terrain_passable(t, x, y);
    return a == d && b == c && a != b;
  };

  for (s64 y = 0; y <= ch; ++y)
  {
    s64 x0 = -1;
    for (s64 x = 0; x <= cw; ++x)
    {
      b32 wall  = x < cw && terrain_passable(t, x, y - 1) != terrain_passable(t, x, y);
      b32 split = wall && x0 >= 0 && checker(x, y);

      if (x0 >= 0 && (!wall || split)) {
        append(&result, Wall{ terrain_vertex(t, x0, y), terrain_vertex(t, x, y) });
        x0 = -1;
      }
      if (wall && x0 < 0) x0 = x;
    }
  }

  for (s64 x = 0; x <= cw; ++x)
  {
    s64 y0 = -1;
    for (s64 y = 0; y <= ch; ++y)
    {
      b32 wall  = y < ch && terrain_passable(t, x - 1, y) != terrain_passable(t, x, y);
      b32 split = wall && y0 >= 0 && checker(x, y);

      if (y0 >= 0 && (!wall || split)) {
        append(&result, Wall{ terrain_vertex(t, x, y0), terrain_vertex(t, x, y) });
        y0 = -1;
      }
      if (wall && y0 < 0) y0 = y;
    }
  }

  return result;
}

void
navmesh_init(Navmesh *navmesh)
{
  f64 t0 = time_seconds();

  // :( ?
  navmesh->offset_in_game_state = offsetof(Game_State, navmesh);

  Terrain *t = terrain_from_navmesh(navmesh);
  R_ASSERT(t);

  // Initialize cdt context with super-triangle enclosing the terrain.
  cdt_context ctx = {};
  f32 dim_x = t->cells_x*t->cell_size;
  f32 dim_y = t->cells_y*t->cell_size;
  f32 center_x = t->corner_x + dim_x * 0.5f;
  f32 center_y = t->corner_y + dim_y * 0.5f;
  f32 rx = dim_x * 3.f;
  f32 ry = dim_y * 3.f;
  cdt_init(&ctx, 
           center_x - rx, center_y - ry, 
           center_x + rx, center_y - ry, 
           center_x, center_y + ry);

  // Collect all the walls (cliffs) and insert them.
  Array<Wall> walls = collect_walls(navmesh);
  for (u64 i = 0; i < walls.count; ++i) {
    Wall *w = walls.data + i;
    cdt_insert(&ctx, navmesh->next_id++, w->a.x, w->a.y, w->b.x, w->b.y);
  }

  // Log
  f64 elapsed = time_seconds() - t0;
  log_info(S("Navmesh generated in %.2f seconds."), (f32)elapsed);
}

void
navmesh_clear(Navmesh *navmesh)
{
  navmesh->next_id = 0;
}

void
navmesh_deinit(Navmesh *navmesh)
{
}

void
pathfind(Navmesh *navmesh, vec2 src, vec2 dst, f32 radius)
{
  ProfileScope;
}
