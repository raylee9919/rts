// Copyright Seong Woo Lee. All Rights Reserved.

#include "./terrain.h"
#include "basic/log.h"
#include "gfx/gfx.h"
#include "game.h"
#include "asset/asset.h"


#define TERRAIN_CLIFF_HEIGHT       4.f
#define TERRAIN_MAX_SLOPE_DEGREES 45.f // Cells steeper than this are impassable.


static b32
split_00_11(f32 h00, f32 h10, f32 h01, f32 h11)
{
  return m_abs(h00 - h11) <= m_abs(h10 - h01);
}

static Game_State *
game_state_from_terrain(Terrain *t)
{
  return (Game_State*)((u8*)t - t->offset_in_game_state);
}

static b8 *
get_passables(Terrain *terrain)
{
  Game_State *g = game_state_from_terrain(terrain);
  return (b8*)pointer_from_handle(g, terrain->passables);
}

void
terrain_init(Game_State *g, 
             Terrain    *terrain, 
             Guid        height_map,
             f32         height_scale,
             f32         cell_size)
{
  f64 t0 = time_seconds();

  // @Temporary
  s64 cell_count_x = 256;
  s64 cell_count_y = 256;

  terrain->offset_in_game_state = offsetof(Game_State, terrain);

  terrain->vertices_x = cell_count_x + 1;
  terrain->vertices_y = cell_count_y + 1;
  terrain->cells_x    = cell_count_x;
  terrain->cells_y    = cell_count_y;
  terrain->cell_size  = cell_size;
  terrain->corner_x   = -0.5f * cell_count_x * cell_size;
  terrain->corner_y   = -0.5f * cell_count_y * cell_size;

  // alias
  s64 vert_x = terrain->vertices_x;
  s64 vert_y = terrain->vertices_y;
  s64 cell_x = cell_count_x;
  s64 cell_y = cell_count_y;


  //
  // Compute heights
  //
  f32 *heights = (f32*)game_alloc(g, sizeof(f32)*vert_x*vert_y, align_of(f32));
  terrain->heights = handle_from_pointer(g, heights);

  // @Temporary: Generate heights
  for (s64 y = 0; y < vert_y; ++y) {
    for (s64 x = 0; x < vert_x; ++x) {
      s64 index = terrain->vertices_x*y + x;
      heights[index] = height_scale * 0.5f * (m_sin(x * 0.05f) * m_cos(y * 0.05f) + 1.f);
    }
  }


  //
  // Compute passable cells by checking slopes of 
  // two triangles in each cell, following the split rule.
  //
  b8 *passables = (b8*)game_alloc(g, sizeof(b8)*cell_x*cell_y, align_of(b8));
  terrain->passables = handle_from_pointer(g, passables);
  for (s64 y = 0; y < cell_y; ++y)
  {
    for (s64 x = 0; x < cell_x; ++x)
    {
      f32 h00 = heights[y*vert_x + x];
      f32 h10 = heights[y*vert_x + x + 1];
      f32 h01 = heights[(y+1)*vert_x + x];
      f32 h11 = heights[(y+1)*vert_x + x + 1];

      f32 a, b;

      auto normal = [&](f32 du, f32 dv) {
        return cell_size / m_sqrt(du*du + dv*dv + cell_size*cell_size);
      };

      if (split_00_11(h00, h10, h01, h11)) {
        a = normal(h11 - h01, h01 - h00);
        b = normal(h10 - h00, h11 - h10);
      } else {
        a = normal(h10 - h00, h01 - h00);
        b = normal(h11 - h01, h11 - h10);
      }

      f32 max_slop_cos = m_cos(radian_from_degree(TERRAIN_MAX_SLOPE_DEGREES));
      passables[y*cell_x + x] = (a >= max_slop_cos && b >= max_slop_cos);
    }
  }


  //
  // Compute array of discrete levels of the cells 
  // from the given heights.
  //
  u16 *levels = (u16*)game_alloc(g, sizeof(u16)*cell_x*cell_y, align_of(u16));
  terrain->levels = handle_from_pointer(g, levels);
  for (s64 y = 0; y < cell_y; ++y) {
    for (s64 x = 0; x < cell_x; ++x) {
      f32 h00   = heights[y*vert_x + x];
      f32 h10   = heights[y*vert_x + x + 1];
      f32 h01   = heights[(y+1)*vert_x + x];
      f32 h11   = heights[(y+1)*vert_x + x + 1];
      f32 h     = min(min(h00, h10), min(h01, h11));
      s32 level = clamp((s32)(h / TERRAIN_CLIFF_HEIGHT), 0, (s32)UINT16_MAX);
      levels[y*cell_x + x] = level;
    }
  }


  // @Temporary
  terrain->material_id = guid_from_string(S("terrain/terrain.material"));
  asset_request(terrain->material_id, NULL_GUID, NULL);

  terrain->initted = true;

  // Log
  f64 elapsed = time_seconds() - t0;
  log_info(S("%dx%d terrain generated in %.2f seconds."), (int)cell_x, (int)cell_y, (f32)elapsed);
}

void 
terrain_deinit(Terrain *terrain)
{
  memset(terrain, 0, sizeof(Terrain));
  // @Todo: release stuffs.
}

f32
terrain_height(Game_State *g, f32 x, f32 y)
{
  Terrain *terrain = &g->terrain;

  f32 *heights = (f32*)pointer_from_handle(g, terrain->heights);
  if (!heights) return 0.f;

  s64 Xv = terrain->vertices_x;
  s64 Yv = terrain->vertices_y;

  f32 Xf = clamp((x - terrain->corner_x) / terrain->cell_size, 0.f, (f32)(Xv - 1));
  f32 Yf = clamp((y - terrain->corner_y) / terrain->cell_size, 0.f, (f32)(Yv - 1));

  s64 Xi = min((s64)Xf, Xv - 2);
  s64 Yi = min((s64)Yf, Yv - 2);

  f32 u = Xf - (f32)Xi;
  f32 v = Yf - (f32)Yi;

  f32 h00 = heights[Yi*Xv + Xi];
  f32 h10 = heights[Yi*Xv + Xi + 1];
  f32 h01 = heights[(Yi+1)*Xv + Xi];
  f32 h11 = heights[(Yi+1)*Xv + Xi + 1];

  if (split_00_11(h00, h10, h01, h11)) {
    if (v >= u) {
      return h00 + u*(h11-h01) + v*(h01-h00);
    } else {
      return h00 + u*(h10-h00) + v*(h11-h10);
    }
  } else {
    if (u + v <= 1.f) {
      return h00 + u*(h10-h00) + v*(h01-h00);
    } else {
      return h11 + (1.f-u)*(h01-h11) + (1.f-v)*(h10 - h11);
    }
  }
}

b32
terrain_contains(Terrain *terrain, f32 x, f32 y)
{
  f32 x_end = terrain->corner_x + terrain->cell_size * (f32)(terrain->cells_x);
  f32 y_end = terrain->corner_y + terrain->cell_size * (f32)(terrain->cells_y);
  return x >= terrain->corner_x && y >= terrain->corner_y && x <= x_end && y <= y_end;
}

b8
terrain_passable(Terrain *terrain, s32 cell_x, s32 cell_y)
{
  if (cell_x < 0 || cell_x >= terrain->cells_x || 
      cell_y < 0 || cell_y >= terrain->cells_y)
    return 0;

  b8 *passables = get_passables(terrain);
  return passables[terrain->cells_x*cell_y + cell_x];
}

vec2
terrain_vertex(Terrain *terrain, s32 vertex_x, s32 vertex_y)
{
  return vec2(terrain->corner_x + vertex_x*terrain->cell_size,
              terrain->corner_y + vertex_y*terrain->cell_size);
}
