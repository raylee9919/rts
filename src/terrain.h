// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_TERRAIN_H
#define RTS_TERRAIN_H

#include "os/os.h"
#include "handle.h"

struct Game_State;

struct Terrain
{
  s64     offset_in_game_state; // @Cleanup: Not fan of this.

  b32     initted;

  Handle  heights;
  Handle  passables;  // Array of booleans representing passability of each cell
  Handle  levels;     // Array of discrete level for each cell

  s64     vertices_x;
  s64     vertices_y;

  s64     cells_x;
  s64     cells_y;
  f32     cell_size;

  // @Temporary
  Guid    material_id;

  // world-space coordinates of the corner.
  f32     corner_x;
  f32     corner_y;
};


void
terrain_init(Game_State *game_state,
             Terrain    *terrain, 
             Guid        height_map,
             f32         height_scale,
             f32         cell_size);

void 
terrain_deinit(Terrain *terrain);

f32
terrain_height(Game_State *g, f32 x, f32 y);

b32
terrain_contains(Terrain *terrain, f32 x, f32 y);

b8
terrain_passable(Terrain *terrain, s32 cell_x, s32 cell_y);

vec2
terrain_vertex(Terrain *terrain, s32 vertex_x, s32 vertex_y);


#endif // RTS_TERRAIN_H
