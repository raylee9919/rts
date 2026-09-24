// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/core.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/string.h"
#include "basic/context.h"
#include "math/math.h"
#include "os/os.h"
#include "gfx/gfx.h"
#include "renderer/renderer.h"
#include "profiler/profiler.h"
#include "game.h"
#include "asset/asset.h"
#include "asset/mesh.h"
#include "animation/animation.h"
#include "audio/audio.h"
#include "shared.h"
#include "material/material.h"
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

    // Advance animations. Players shared between entities advance once.
    entity_dfs(g, g->root, &dt, [](Game_State *g, Entity *E, u64 i, void *data) {
        Animation_Player *player = animation_player_from_offset(g, E->animation_player);
        if (player) {
            animation_player_update(g, player, (f32)*(f64 *)data);
        }
    });

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
    // Init shared state. Threads share this state.
    shared_init();

    // Init timers
    f64 time_old        = time_seconds();
    f64 dt              = 1.0 / 60.0; // Update frequency, Tick rate
    f64 accumulator     = 0.f;

    // Init asset system.
    // Register asset types and init catalog.
    asset_system_init();

    // Init material system.
    // Reflect and construct material type table.
    String material_shader_path = tprint(S("%S/%S"), shared->data_path, S("shaders/material/"));
    material_system_init(material_shader_path, shared->shader_compiler);

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

    // Init camera
    game_state->camera.position = vec3(0.f, 0.7f, 1.6f); // Framed on the knight.


    // @Temporary
    // Create and initalize entities. This is a placeholder.
    // Entities are assets, thus will be loaded from the disk as well.
    {
        // Submesh name -> material. The mesh carries no material reference itself,
        // so the binding lives here until entities become assets of their own.
        struct Knight_Material { String mesh; String material; };
        Knight_Material knight_materials[] = {
            { S("Helm2"),        S("materials/knight/helm.material")         },
            { S("Arms"),         S("materials/knight/arms.material")         },
            { S("Acessories"),   S("materials/knight/arms.material")         },
            { S("Acessories2"),  S("materials/knight/arms.material")         },
            { S("Breast_Armor"), S("materials/knight/breast_armor.material") },
            { S("Leegs_Armor1"), S("materials/knight/breast_armor.material") },
            { S("pants"),        S("materials/knight/breast_armor.material") },
            { S("Weapon2"),      S("materials/knight/sword.material")        },
            { S("Shield"),       S("materials/knight/shield.material")       },
        };

        Guid knight_id = guid_from_string(S("mesh/knight.triangle_mesh"));
        asset_request(knight_id);

        Asset::Model *knight = Asset::model_from_guid(knight_id);
        R_ASSERT(knight);

        Guid skeleton_id = guid_from_string(S("skeleton/knight.skeleton"));
        Guid idle_id     = guid_from_string(S("animation/knight_idle.keyframed_animation"));
        asset_request(skeleton_id);
        asset_request(idle_id);

        Asset::Skeleton  *skeleton = Asset::skeleton_from_guid(skeleton_id);
        Asset::Animation *idle     = Asset::animation_from_guid(idle_id);
        R_ASSERT(skeleton && idle);

        // Every submesh is skinned to the same skeleton, so they share one player.
        u64 knight_player = animation_player_alloc(game_state, skeleton);
        animation_player_set(animation_player_from_offset(game_state, knight_player), 0, idle, true, 1.f);

        for (u32 i = 0; i < knight->num_meshes; ++i) {
            Asset::Mesh *mesh = &knight->meshes[i];

            String material_name = {};
            for (Knight_Material &km : knight_materials) {
                if (km.mesh == mesh->name) { material_name = km.material; break; }
            }

            if ( !material_name.str ) {
                log_warning(S("No material for knight submesh '%S', skipping."), mesh->name);
                continue;
            }

            Guid material = guid_from_string(material_name);
            asset_request(material);

            Entity *E   = entity_alloc(game_state);
            E->mesh     = mesh->gpu_id;
            E->material = material;

            // The knight was authored in centimetres. The skeleton's root transform
            // (0.01 uniform) is baked into the skinning matrices and brings him to scale.
            E->animation_player = knight_player;
        }
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
