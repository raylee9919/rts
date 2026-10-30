// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PASS_GEOMETRY_H
#define RTS_PASS_GEOMETRY_H


#include "renderer/renderer.h"

struct R_Pass_Geometry : R_Pass
{
  #include "shaders/pass/surface.h"

  u32                  skinning_used; // Reset every frame please.

  #include "shaders/pass/terrain.h"
  b32                  terrain_initted;
  Guid                 terrain_mesh_id;
  Guid                 terrain_pipeline;
};

R_PASS_DECLARE_PROCS( Geometry );


#endif // RTS_PASS_GEOMETRY_H
