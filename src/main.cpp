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

// Development
#include "development/console.h"
#include "importer/fbx.h"

// Third Party
#include "third_party/xxhash3/xxhash.h"

/*
 @Todo

 [Renderer]
   - Vertex quantization (with bindless/vertex pulling?)
   - Upload buffer (command list/allocator)
   - Per-frame resource, memory allocation strategy
   - Resource alloc/release
   - BC Texture upload/conversion
   - Deferred pipeline?
   - Transparent material

 [Asset]
   - As everything is GUID, I think I can do "immediate-mode" asset, where 
     the user doesn't have to care about the lifetime?

 [Memory]
   - I need heaps! not a single global heap. `mimalloc`? Roll my own? ;)
   - Pass allocator when initializing subsystems. Better that way.

 [Build]
   - Not urgent, but I want to ditch CMake and do my own build system at some point,
     because CMake is a crime against humanity.

 [Development]
   - I'm not a fan of separate editor binary, like Unreal.
     I want developemnt tools to be embedded and be hoisted out cleanly 
     in shipping build.

 - I had no idea asset loading function pointers would be hard to debug...
 - Shader compiler debug option
 - Not fond of guid generation...
 - Cull terrain by "chunk" granularity.
 - Color
 - SIMD CPUID and bind corresponding functions.
 - Trigonometry
 - pointer, game storage, and saving games...

 [Low Priority]
   - force inline math?
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
    Handle h = handle_from_entity(g, e);
    f32 u = (f32)h.generational_id;
    e->position.x = m_cos(g->time*m_sin(u))*0.25f*u;
    e->position.y = m_sin(g->time*m_sin(u))*0.25f*u; 
    e->position.z  = terrain_height(game_state, e->position.x, e->position.y);

    AABBV tight_box = aabbv_translate(e->tight_box, e->position);
    if (!aabbv_contains(&e->loose_box_world, &tight_box))
    {
      e->loose_box_world = aabbv_translate(e->loose_box_local, e->position);
      aabb_tree_move(&g->aabb_tree, e->aabb_tree_proxy, e->loose_box_world);
    }
  });
}

void 
per_frame_tick(Game_State *g, f64 dt)
{
  static b32 t = false;
  if (t)
  {
    aabb_tree_visualize(&g->aabb_tree);
  }

  if (key_transitioned_down(&g->inputs, KEY_F1))
  {
    t = !t;
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
  shared->window = window_create(1280, 720, S("RTS"), true);

  // Launch render thread
  Thread render_thread = thread_launch(r_entry, get_native_window_handle(shared->window));

  // Launch audio thread
  Thread audio_thread = thread_launch(audio_entry, NULL);

  // Spin-lock until renderer is initted.
  for (;;) 
  {
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

  for (s32 i = 0; i < 256; ++i) 
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
    my_entity->tight_box = aabbv_make(-0.52f, -0.25f, -0.01f, 0.52f, 0.28f, 1.6f);
    my_entity->loose_box_local = my_entity->tight_box;
    aabbv_expand(&my_entity->loose_box_local, 0.50f);

    // @Todo: Scale too?
    my_entity->loose_box_world = aabbv_translate(my_entity->loose_box_local, my_entity->position);
    my_entity->aabb_tree_proxy = aabb_tree_insert(&game_state->aabb_tree, handle_from_entity(game_state, my_entity), my_entity->loose_box_world);
  }
  // @Temporary
  // @Temporary
  // @Temporary


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
    // @Todo: Ditch floating point?
    while (accumulator >= dt) {
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
