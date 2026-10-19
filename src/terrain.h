// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_TERRAIN_H
#define RTS_TERRAIN_H

#include "os/os.h"
#include "math/math.h"
#include "handle.h"

struct Game_State;

struct Terrain
{
  b32     initted;

  Guid    height_map_id;
  Handle  heights;

  s64     vertices_x;
  s64     vertices_y;

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
             s64         cell_count_x, 
             s64         cell_count_y,
             f32         cell_size);


#endif // RTS_TERRAIN_H
