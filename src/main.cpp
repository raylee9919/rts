// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/core.h"
#include "basic/allocator.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/string.h"
#include "basic/context.h"
#include "math/math.h"
#include "os/os.h"
#include "geometry/geogen.h"
#include "rhi/rhi.h"
#include "gfx/gfx.h"
#include "renderer/renderer.h"
#include "profiler/profiler.h"
#include "game.h"
#include "asset/asset.h"
#include "audio/audio.h"
#include "shared.h"
#include "material.h"
#include "third_party/xxhash3/xxhash.h"


//
struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

Vertex  vertices[24];
u32     indices[36];


u32 num_vertices = array_count(vertices);
u32 num_indices  = array_count(indices);




void game_tick(Game_State *g, f64 dt) 
{
    ProfileScope;

    g->time += dt;

    {
#if 0
        static bool initted = false;
        String name = S("CameraMoveForward");

        if (!initted) {
            Input_Action action = {};
            table_add(&input_system.action_table, name, action);
            initted = true;
        }

        Input_Value val;
        if (input_action(name, &val)) {

        }
#endif
    }


    {
        Camera *camera = &g->camera;

        f32 movement_speed = 10.f;
        f32 turn_speed     = 0.25f;
    }


#if 0
        static v2 p0 = {};
        if (event->kind == OS_EVENT_PRESS && event->key == KEY_MOUSE_LEFT && event->window == window) {
            os_remove_event(event);
            p0 = event->position;
        }

        if (g->input_state.key_is_down[KEY_MOUSE_LEFT]) {
            v2 p1 = os_get_mouse_position(window);
            v2 dp = p1 - p0;

            camera->yaw   -= dt * turn_speed * dp.x;
            camera->pitch -= dt * turn_speed * dp.y;
            camera->pitch = clamp(camera->pitch,  -pi32 * 0.25f, pi32 * 0.25f);

            p0 = p1;
        }
#endif

#if 0
        if (g->input_state.key_is_down[KEY_W]) {
            camera->position += dt * movement_speed * (x_rotation(camera->pitch) * y_rotation(camera->yaw) * FORWARD_VECTOR).xyz;
        }

        if (g->input_state.key_is_down[KEY_S]) {
            camera->position -= dt * movement_speed * (x_rotation(camera->pitch) * y_rotation(camera->yaw) * FORWARD_VECTOR).xyz;
        }

        if (g->input_state.key_is_down[KEY_A]) {
            camera->position -= dt * movement_speed * (y_rotation(camera->yaw) * RIGHT_VECTOR).xyz;
        }

        if (g->input_state.key_is_down[KEY_D]) {
            camera->position += dt * movement_speed * (y_rotation(camera->yaw) * RIGHT_VECTOR).xyz;
        }

        if (g->input_state.key_is_down[KEY_E]) {
            camera->position += dt * movement_speed * UP_VECTOR.xyz;
        }

        if (g->input_state.key_is_down[KEY_Q]) {
            camera->position -= dt * movement_speed * UP_VECTOR.xyz;
        }
#endif

    entity_dfs(g, g->root, nullptr, [](Game_State *g, Entity *entity, u64 index, void *user_data) {
        f32 spacing = 3.0f;

        u32 x = index % 10;
        u32 y = (index / 10) % 10;
        u32 z = index / 100;

        entity->position.x = ((f32)x - 4.5f) * spacing;
        entity->position.y = ((f32)y - 4.5f) * spacing;
        entity->position.z = ((f32)z - 4.5f) * spacing;
    });
}

int main_entry(int argc, char **argv)
{
    // Init shared state. Threads share this state.
    shared_init();

    // Init timers
    f64 time_old        = time_seconds();
    f64 dt              = 1.0 / 60.0; // Update frequency, Tick rate
    f64 accumulator     = 0.f;

    // Init asset system.
    // Register asset types and init catalog.
    asset_system_init();

    // Init Game
    game_init(time_old);

    // Open window
    shared->window = window_create(1600, 900, S("RTS"));

    // Launch render thread
    Thread render_thread = thread_launch(r_entry, get_native_window_handle(shared->window));

    // Launch audio thread
    Thread audio_thread = thread_launch(audio_entry, NULL);

    // Spin-lock until renderer is initted.
    while (!renderer || !renderer->initted) {}

    // Init camera
    game_state->camera.position = vec3(0.f, 6.f, 15.f);

    // Make a cube
    cube_mesh._64[0] = 7474; // @Temporary
    geo_make_cube(vertices, sizeof(Vertex), offset_of(Vertex, position), offset_of(Vertex, normal), offset_of(Vertex, uv), indices, sizeof(indices[0]));


    {
        gfx_mesh_create(cube_mesh, vertices, num_vertices, sizeof(vertices[0]), indices, num_indices, sizeof(indices[0]));


        { // Create camera buffer and view
            u64 stride = sizeof(GPU_Camera);
            u64 sz     = sizeof(GPU_Camera) * 1;

            RHI_Buffer_Desc desc = {};
            desc.memory_type = RHI_MEMORY_UPLOAD;
            desc.size        = sz;

            Assert(rhi_buffer_init(gfx->device, &camera_buffer, &desc, NULL));

            RHI_Buffer_View_Desc view_desc = {};
            {
                view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
                view_desc.writable = false;
                view_desc.stride   = stride;
                view_desc.offset   = 0;
                view_desc.size     = sz;
            }

            rhi_buffer_view_init(gfx->device, &camera_view, &camera_buffer, &view_desc);

            camera_ptr = rhi_buffer_map(&camera_buffer);
        }
    }


    // @Temporary
    // Create and initalize entities. This is a placeholder.
    // Entities are assets, thus will be loaded from the disk as well.
    for (int i = 0; i < 256; ++i) 
    {
        Entity *E = entity_alloc(game_state);
        E->mesh     = cube_mesh;
        E->material = guid_from_string(S("materials/doggo.material")); // @Temporary

        asset_request(E->material); // @Temporary
    }
    
    // Game loop
    while ( !shared->should_close ) 
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
        { ProfileScopeN("InputProcessing");
            update_window_events();
        }


        // Tick with fixed timestep
        for (u32 counter = 0; accumulator >= dt && counter < 10; counter += 1) {
            ProfileScopeN("TickGame");
            accumulator -= dt;
            game_tick(game_state, dt);
        }


        { // Push state to render thread
            auto *ring = &renderer->ring;

            Assert(!ring->is_full());
            auto* entry = &ring->entries[ring->write_idx];

            mutex_lock(&entry->mutex);

            game_copy(entry->game_state, game_state);
            ring->write_idx = (ring->write_idx + 1) % array_count(ring->entries);

            mutex_unlock(&entry->mutex);
        }

#if 0
        list_for(os->first_event, event)  {
            b32 esc_pressed            = event->kind == OS_EVENT_PRESS && event->key == KEY_ESC;
            b32 alt_f4_pressed         = event->kind == OS_EVENT_PRESS && event->key == KEY_F4 && (event->modifiers & OS_MODIFIER_ALT);
            b32 window_close_triggered = event->kind == OS_EVENT_WINDOW_CLOSE && event->window == window;

            if (esc_pressed || alt_f4_pressed | window_close_triggered) {
                os_remove_event(event);

                should_close = true;

                // Shutdown render thread
                mutex_lock(&render_queue.mutex);
                gfx->should_shutdown = true;
                condvar_wake_all(&render_queue.condvar);
                mutex_unlock(&render_queue.mutex);
            }

            // Fullscreen
            b32 alt_enter_pressed = event->kind == OS_EVENT_PRESS && event->key == KEY_RETURN && (event->modifiers & OS_MODIFIER_ALT);
            if (alt_enter_pressed) {
                os_window_toggle_fullscreen(window);
            }
        }
#endif

        clear_thread_temporary_storage();
    }


    /* Join Threads */
    thread_join(audio_thread,  -1);
    thread_join(render_thread, -1);


    /* Shutdown Systems */
    game_deinit();
    asset_system_shutdown();

    return 0;
}
