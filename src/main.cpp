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

/* Development */
#include "development/console.h"
#include "importer/fbx.h"

/* Third Party */
#include "third_party/xxhash3/xxhash.h"

// @Todo
// - I had no idea asset loading function pointers would be hard to debug...
// - Shader compiler debug option
// - Not fond of guid generation...


static b8 key_w = 0;
static b8 key_a = 0;
static b8 key_s = 0;
static b8 key_d = 0;
static b8 key_q = 0;
static b8 key_e = 0;


void game_tick(Game_State *g, f64 dt) 
{
    ProfileScope;

    g->time += dt;

    camera_update(&g->camera, dt);
}

void input_process()
{
    ProfileScope;

    update_window_events();

    for (Event& event : os->events) {

        if (event.type == EVENT_KEYBOARD) {
            if (event.key_code == KEY_ENTER && event.modifier_flags.alt_pressed && event.key_pressed) {
                toggle_fullscreen(shared->window);
            }

            if (event.key_code == KEY_F4 && event.modifier_flags.alt_pressed) {
                shared->should_close = true;
            }

            if (event.key_code == 'W')  key_w = event.key_pressed;
            if (event.key_code == 'A')  key_a = event.key_pressed;
            if (event.key_code == 'S')  key_s = event.key_pressed;
            if (event.key_code == 'D')  key_d = event.key_pressed;
            if (event.key_code == 'Q')  key_q = event.key_pressed;
            if (event.key_code == 'E')  key_e = event.key_pressed;

        } else if (event.type == EVENT_DRAG_AND_DROP_FILES) {

        }

    }
}

int 
main_entry(int argc, char **argv)
{
  shared_init();

  // Init timers
  f64 time_old    = time_seconds();
  f64 dt          = 1.0 / 60.0; // Update frequency, Tick rate
  f64 accumulator = 0.f;

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
    _mm_pause();
  }

  // Init material system.
  // Reflect and construct material type table.
  String material_shader_path = tprint(S("%S/%S"), shared->data_path, S("shaders/material/"));
  material_system_init(material_shader_path, shared->shader_compiler);

  // Init Game
  game_init(time_old);



  // @Temporary
  // @Temporary
  // @Temporary
#if 1
  FBX_Import_Options options = {};
  options.generate_normals_with_mesh_optimizer = true;
  options.overwrite_existing_file = true;
  R_ASSERT( fbx_import(S("C:/Arts/Goblin/Mixamo/OrcWalk.fbx"), S(""), options) );
#endif
  Guid mesh_id = guid_from_string(S("goblin/Goblin.triangle_mesh"));
  Entity *my_entity = entity_alloc(game_state);
  my_entity->mesh = mesh_id;
  asset_request(mesh_id, NULL_GUID, NULL);

  my_entity->animation_id = guid_from_string(S("goblin/OrcWalk.sampled_animation"));
  asset_request(my_entity->animation_id, NULL_GUID, NULL);

  my_entity->position.z = terrain_height(game_state, 0, 0);
  // @Temporary
  // @Temporary
  // @Temporary


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
    f64 time_new        = time_seconds();
    f64 time_elapsed    = time_new - time_old;
    time_old            = time_new;
    accumulator        += time_elapsed;


    // Input processing
    input_process();


    // Tick with fixed timestep
    for (u32 counter = 0; accumulator >= dt && counter < 10; counter += 1) {
      ProfileScopeN("TickGame");
      accumulator -= dt;
      game_tick(game_state, dt);
    }



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
