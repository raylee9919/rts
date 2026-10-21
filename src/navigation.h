// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_NAVIGATION_H
#define RTS_NAVIGATION_H

#include "terrain.h"

struct Navmesh
{
  s64 offset_in_game_state; // @Cleanup
  u32 next_id;
};

void
navmesh_init(Navmesh *navmesh);

void
navmesh_deinit(Navmesh *navmesh);


#endif // RTS_NAVIGATION_H
