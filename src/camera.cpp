// Copyright Seong Woo Lee. All Rights Reserved.

#include "./camera.h"
#include "game.h"
#include "renderer/renderer.h"

#define CAMERA_FOV           0.70f
#define CAMERA_MIN_DISTANCE    7.f
// #define CAMERA_MAX_DISTANCE   70.f
#define CAMERA_MAX_DISTANCE  300.f
#define CAMERA_PITCH_NEAR   -0.60f
#define CAMERA_PITCH_FAR    -0.98f
#define CAMERA_SMOOTHING      12.f

static Game_State *
game_state_from_camera(Camera *camera)
{
  Game_State *g = (Game_State *)((u8*)camera - camera->offset_in_game_state);
  return g;
}

static f32
pitch_from_distance(f32 d)
{
  f32 t = (d - CAMERA_MIN_DISTANCE) / (CAMERA_MAX_DISTANCE - CAMERA_MIN_DISTANCE);
  return lerp(CAMERA_PITCH_NEAR, CAMERA_PITCH_FAR, smoothstep(0.f, 1.f, t));
}

static void
update_pose(Camera *camera)
{
  // Update pitch and position from distance and focus.
  camera->pitch = pitch_from_distance(camera->distance);
  vec3 fwd = camera_forward(camera);
  camera->position = camera->focus - fwd * camera->distance;

  // Always stay above ground!
  Game_State *g = game_state_from_camera(camera);
  f32 height = terrain_height(g, camera->position.x, camera->position.y);
  camera->position.z = max(height + 1.5f, camera->position.z);
}

static f32
sample_heights(Game_State *g, f32 x, f32 y)
{
  f32 h = 0.f;
  f32 r = 3.f;
  h += terrain_height(g,     x,     y) * 2.f;
  h += terrain_height(g, x + r,     y);
  h += terrain_height(g, x - r,     y);
  h += terrain_height(g,     x, y + r);
  h += terrain_height(g,     x, y - r);
  return h / 6.f;
}

static void
clamp_focus(Camera *camera, vec3 *focus)
{
  Game_State *g = game_state_from_camera(camera);
  Terrain *t = &g->terrain;

  f32 margin = 4.f;
  f32 x0 = t->corner_x + margin;
  f32 y0 = t->corner_y + margin;
  f32 x1 = t->corner_x + t->cell_size * (f32)(t->vertices_x - 1) - margin;
  f32 y1 = t->corner_y + t->cell_size * (f32)(t->vertices_y - 1) - margin;

  focus->x = clamp(focus->x, x0, x1);
  focus->y = clamp(focus->y, y0, y1);
  focus->z = sample_heights(g, focus->x, focus->y);
}

void 
camera_init(Game_State *g, 
            Camera     *camera, 
            vec3        focus, 
            f32         yaw, 
            f32         distance)
{
  camera->offset_in_game_state = offsetof(Game_State, camera);

  camera->fov             = CAMERA_FOV;

  clamp_focus(camera, &focus);
  camera->focus           = focus;
  camera->focus_target    = focus;

  camera->yaw             = yaw;
  camera->yaw_target      = yaw;

  distance = clamp(distance, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE);
  camera->distance        = distance;
  camera->distance_target = distance;

  update_pose(camera);
}

void
camera_tick(Camera *camera, f32 dt)
{
  clamp_focus(camera, &camera->focus_target);

  camera->distance_target = clamp(camera->distance_target, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE);
  camera->distance = clamp(camera->distance, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE);

  // Smooth in
  f32 a = 1.f - m_exp(-CAMERA_SMOOTHING * dt);
  camera->focus    = lerp(camera->focus, camera->focus_target, a);
  camera->distance = lerp(camera->distance, camera->distance_target, a);
  camera->yaw      = lerp(camera->yaw, camera->yaw_target, a);

  update_pose(camera);

  f32 aspect_ratio = (f32)RESOLUTION_X / (f32)RESOLUTION_Y;
  vec3 fwd = camera_forward(camera);

  camera->view      = look_to_rh(camera->position, fwd, WORLD_UP);
  camera->proj      = persp_fov_rh(camera->fov, aspect_ratio, NEAR_Z, FAR_Z);
  camera->view_proj = camera->proj * camera->view;
}

vec3 
camera_forward(Camera *camera)
{
  return normalize((z_rotation(camera->yaw) * x_rotation(camera->pitch) * FORWARD_VECTOR).xyz);
}

void 
camera_look_at(Camera *camera, vec3 focus)
{
  camera->focus_target = focus;
  clamp_focus(camera, &camera->focus_target);
}

vec4 
camera_project(Camera *camera, vec3 p)
{
  vec4 q = camera->view_proj * vec4(p, 1.f);
  return q;
}
