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
#include "asset_lexer.h"
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


    Camera *camera = &g->camera;

    f32 movement_speed = 10.f;
    f32 turn_speed     = 0.25f;


    if (key_w) {
        camera->position += dt * movement_speed * (x_rotation(camera->pitch) * y_rotation(camera->yaw) * FORWARD_VECTOR).xyz;
    }

    if (key_a) {
        camera->position -= dt * movement_speed * (y_rotation(camera->yaw) * RIGHT_VECTOR).xyz;
    }

    if (key_s) {
        camera->position -= dt * movement_speed * (x_rotation(camera->pitch) * y_rotation(camera->yaw) * FORWARD_VECTOR).xyz;
    }

    if (key_d) {
        camera->position += dt * movement_speed * (y_rotation(camera->yaw) * RIGHT_VECTOR).xyz;
    }

    if (key_q) {
        camera->position -= dt * movement_speed * UP_VECTOR.xyz;
    }

    if (key_e) {
        camera->position += dt * movement_speed * UP_VECTOR.xyz;
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

int main_entry(int argc, char **argv)
{
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
    shared->window = window_create(1920, 1080, S("RTS"), true);

    // Launch render thread
    Thread render_thread = thread_launch(r_entry, get_native_window_handle(shared->window));

    // Launch audio thread
    Thread audio_thread = thread_launch(audio_entry, NULL);

    // Spin-lock until renderer is initted.
    while (!renderer || !renderer->initted) {}

    // Init material system.
    // Reflect and construct material type table.
    String material_shader_path = tprint(S("%S/%S"), shared->data_path, S("shaders/material/"));
    material_system_init(material_shader_path, shared->shader_compiler);

    // Init camera
    game_state->camera.position = vec3(0.f, 0.7f, 1.6f); // Framed on the knight.



    // @Temporary
    // @Temporary
    // @Temporary
#if 1
    FBX_Import_Options options = {};
    options.generate_normals_with_mesh_optimizer = true;
    options.overwrite_existing_file = true;
    R_ASSERT( fbx_import(S("C:/Users/swl/Desktop/rts_assets/input/knight.fbx"), S(""), options) );
#endif
    // @Temporary
    // @Temporary
    // @Temporary


    // @Temporary
    // Create and initalize entities. This is a placeholder.
    // Entities are assets, thus will be loaded from the disk as well.
    {
#if 0
        // Submesh name -> material. The mesh carries no material reference itself,
        // so the binding lives here until entities become assets of their own.
        struct Knight_Material { String mesh; String material; };
        Knight_Material knight_materials[] = {
            { S("Helm2"),        S("materials/knight/helm.material")         },
            { S("Arms"),         S("materials/knight/arms.material")         },
            { S("Accessories"),  S("materials/knight/arms.material")         },
            { S("Accessories2"), S("materials/knight/arms.material")         },
            { S("Breast_Armor"), S("materials/knight/breast_armor.material") },
            { S("Legs_Armor1"),  S("materials/knight/breast_armor.material") },
            { S("Pants"),        S("materials/knight/breast_armor.material") },
            { S("Weapon2"),      S("materials/knight/sword.material")        },
            { S("Shield"),       S("materials/knight/shield.material")       },
        };
#endif

#if 0
        Guid knight_id = guid_from_string();

        Asset::Model *knight = Asset::model_from_guid(knight_id);
        R_ASSERT(knight);
#endif

        Guid mesh_id = guid_from_string(S("knight.triangle_mesh"));
        Entity *knight = entity_alloc(game_state);
        knight->mesh = mesh_id;
        asset_request(mesh_id);
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
