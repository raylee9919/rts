// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_CAMERA_H
#define RTS_CAMERA_H

#include "math/math.h"

struct Game_State;

/*
    Input -> Pan    -> 'focus_target'    changes
          -> Zoom   -> 'distance_target' changes
          -> Rotate -> 'yaw_target'      changes

    'focus', 'yaw', 'distance' smoothly approch toward each target.

    'distance' -> 'pitch'
    'focus', 'distance' -> 'position'
*/
struct Camera 
{
  s64  offset_in_game_state; // @Cleanup: Not fan of this.

  f32  fov;

  vec3 focus;
  vec3 focus_target;

  f32  yaw;
  f32  yaw_target;

  f32  distance;
  f32  distance_target;

  vec3 position;
  f32  pitch;

  m4x4 view;
  m4x4 proj;
  m4x4 view_proj;
  m4x4 inv_view;
  m4x4 inv_proj;
  m4x4 inv_view_proj;
};

void 
camera_init(Game_State *g, Camera *camera, vec3 focus, f32 yaw, f32 distance);

void
camera_tick(Camera *camera, f32 dt);

// Returns normalized forward vector of the camera.
vec3 
camera_forward(Camera *camera);

// returns clip-space coordinates.
vec4 
camera_project(Camera *camera, vec3 p);


#endif // RTS_CAMERA_H
