// Copyright Seong Woo Lee. All Rights Reserved.

#include "profiler/profiler.h"
#include "basic/core.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/string.h"
#include "basic/context.h"
#include "os/os.h"
#include "math/math.h"
#include "shared.h"
#include "renderer/renderer.h"
#include "game.h"
#include "asset/asset.h"
#include "renderer/immediate.h"
#include "audio/audio.h"
#include "material/material.h"
#include "font/font.h"
#include "mesh.h"
#include "random/random.h"

/* Development */
#include "development/console.h"
#include "importer/fbx.h"

/* Third Party */
#include "third_party/xxhash3/xxhash.h"

/*
 @Todo
 - I had no idea asset loading function pointers would be hard to debug...
 - Shader compiler debug option
 - Not fond of guid generation...
 - Cull terrain by "chunk" granularity.
 - Color
 - SIMD CPUID and bind corresponding functions.
 - Trigonometry
 - pointer, game storage, and saving games...
*/

void 
fixed_tick(Game_State *g, f64 dt) 
{
  ProfileScope;
  g->tick += 1;
  g->time += dt;

  Input_State *inputs = &g->inputs;

  {
    Camera *camera = &g->camera;
    f32 c = m_cos(camera->yaw_target);
    f32 s = m_sin(camera->yaw_target);
    vec3 right = vec3( c, s, 0.f);
    vec3 fwd   = vec3(-s, c, 0.f);
    f32 speed  = camera->distance;

    if (key_down(inputs, 'W'))  camera->focus_target +=   fwd * speed * dt;
    if (key_down(inputs, 'S'))  camera->focus_target -=   fwd * speed * dt;
    if (key_down(inputs, 'D'))  camera->focus_target += right * speed * dt;
    if (key_down(inputs, 'A'))  camera->focus_target -= right * speed * dt;

    if (key_down(inputs, 'Q'))  camera->yaw_target += 1.8f * dt;
    if (key_down(inputs, 'E'))  camera->yaw_target -= 1.8f * dt;

    camera->distance_target *= powf(0.85f, inputs->wheel_notches);

    camera_tick(camera, dt);
  }

  entity_dfs(g, g->root, NULL, [](Game_State *g, Entity *e, u64 index, void *data){
    // e->position.x += ((f32)xorshift32() / (f32)UINT32_MAX - 0.5f);
    // e->position.y += ((f32)xorshift32() / (f32)UINT32_MAX - 0.5f);
    e->position.z  = terrain_height(game_state, e->position.x, e->position.y);

    AABBV box = aabbv_translate(e->loose_aabb, e->position);
    aabb_tree_move(&g->aabb_tree, e->aabb_tree_proxy, box);
  });
}

void 
per_frame_tick(Game_State *g, f64 dt)
{
  aabb_tree_visualize(&g->aabb_tree);
}

void 
game_input_process(Game_State *g) 
{
  //
  // Entity selection
  //
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
#if 0
      vec2 _00 = g->game_input.mouse_clicked_postion;
      vec2 _11 = playfield_from_window(x, y);
      vec2 _01 = vec2(_11.x, _00.y);
      vec2 _10 = vec2(_00.x, _11.y);
      Ray ray00 = ray_from_playfield(&g->camera, _00);
      Ray ray01 = ray_from_playfield(&g->camera, _01);
      Ray ray10 = ray_from_playfield(&g->camera, _10);
      Ray ray11 = ray_from_playfield(&g->camera, _11);
      vec3 p00 = terrain_raycast(&g->terrain, ray00);
      vec3 p01 = terrain_raycast(&g->terrain, ray01);
      vec3 p10 = terrain_raycast(&g->terrain, ray10);
      vec3 p11 = terrain_raycast(&g->terrain, ray11);
#endif
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

int 
main_entry(int argc, char **argv)
{
  shared_init();

  // Init asset system.
  // Register asset types and init catalog.
  asset_system_init();

  // Open window
  shared->window = window_create(1920, 1080, S("RTS"), true);

  // Launch render thread
  Thread render_thread = thread_launch(r_entry, get_native_window_handle(shared->window));

  // Launch audio thread
  Thread audio_thread = thread_launch(audio_entry, NULL);

  // Spin-lock until renderer is initted.
  while (1) {
    auto *r = (volatile Renderer *)renderer;
    if (r && r->initted) break;
    _mm_pause(); // @Temporary
  }

  // Init material system.
  // Reflect and construct material type table.
  String material_shader_path = tprint(S("%S/%S"), shared->data_path, S("shaders/material/"));
  material_system_init(material_shader_path, shared->shader_compiler);

  // Init Game
  game_init();



  // @Temporary
  // @Temporary
  // @Temporary
#if 0
  FBX_Import_Options options = {};
  options.generate_normals_with_mesh_optimizer = true;
  options.overwrite_existing_file = true;
  R_ASSERT( fbx_import(S("C:/Arts/Goblin/Mixamo/OrcWalk.fbx"), S(""), options) );
#endif
  Guid mesh_id = guid_from_string(S("goblin/Goblin.triangle_mesh"));

  for (s32 i = 0; i < 64; ++i) 
  {
    Entity *my_entity = entity_alloc(game_state);
    my_entity->mesh = mesh_id;
    asset_request(mesh_id, NULL_GUID, NULL);

    my_entity->animation_id = guid_from_string(S("goblin/OrcWalk.sampled_animation"));
    asset_request(my_entity->animation_id, NULL_GUID, NULL);

    my_entity->position.y = ((f32)xorshift32() / (f32)UINT32_MAX - 0.5f) * 64.f;
    my_entity->position.x = ((f32)xorshift32() / (f32)UINT32_MAX - 0.5f) * 64.f;
    my_entity->position.z = terrain_height(game_state, my_entity->position.x, my_entity->position.y); // @Temporary
    my_entity->radius = 16.f;
    my_entity->tight_aabb = aabbv_make(-0.525789f, -0.253311f, -0.011222f,
                                       0.525789f, 0.282407f, 1.622115f);
    my_entity->loose_aabb = my_entity->tight_aabb;
    aabbv_expand(&my_entity->loose_aabb, 0.25f);



    // @Todo: Scale too?
    AABBV box = aabbv_translate(my_entity->loose_aabb, my_entity->position);
    my_entity->aabb_tree_proxy = aabb_tree_insert(&game_state->aabb_tree, handle_from_entity(game_state, my_entity), box);
  }
  // @temporary
  // @temporary
  // @temporary


  // Timers
  f64 time_old    = time_seconds();
  f64 dt          = 1.0 / 60.0; // Update frequency, Tick rate
  f64 accumulator = 0.f;

  // Game loop
  while (!shared->should_close) 
  {
    ProfileFrameMark;

    // Gaming experience in its finesse
    if (!gfx_wait_for_frame_waitable_object()) {
      log_warning(S("Waiting on frame latency waitable object failed."));
    }


    // For better latency, placing this block here
    { ProfileScopeN("WaitForRenderQueueSpace");
      auto *ring = &renderer->ring;

      mutex_lock(&ring->mutex);

      while (ring->is_full()) {
        condvar_sleep(&ring->condvar, &ring->mutex, -1);
      }

      condvar_wake_all(&ring->condvar);
      mutex_unlock(&ring->mutex);
    }


    // Time
    f64 time_new      = time_seconds();
    f64 time_elapsed  = time_new - time_old;
    time_old          = time_new;
    accumulator      += time_elapsed;


    // Input processing
    update_window_events();
    input_process(&game_state->inputs);
    game_input_process(game_state);


    // Tick with fixed timestep
    for (u32 counter = 0; accumulator >= dt && counter < 10; counter += 1) {
      ProfileScopeN("TickGame");
      accumulator -= dt;
      fixed_tick(game_state, dt);
    }
    per_frame_tick(game_state, time_elapsed);


    { // Push state to render thread
      auto *ring = &renderer->ring;

      R_ASSERT(!ring->is_full());
      auto* entry = &ring->entries[ring->write_idx];

      mutex_lock(&entry->mutex);

      game_copy(entry->game_state, game_state);

      // Copy immediate quads and the counter
      memcpy( entry->quads, immediate_quads, num_immediate_quads * sizeof(immediate_quads[0]) );
      entry->num_quads = num_immediate_quads;
      num_immediate_quads = 0;

      ring->write_idx = (ring->write_idx + 1) % array_count(ring->entries);

      mutex_unlock(&entry->mutex);
    }

    clear_thread_temporary_storage();
  }


  /* Join Threads */
  thread_join(audio_thread,  -1);
  thread_join(render_thread, -1);


  /* Shutdown Systems */
  game_shutdown();
  material_system_shutdown();
  asset_system_shutdown();

  return 0;
}
