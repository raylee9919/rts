// Copyright Seong Woo Lee. All Rights Reserved.

// .h
//
#include "rts_profiler.h"
#include "base/rts_base_inc.h"
#include "os/os.h"
#include "file_system/file.h"
#include "rts_random.h"
#include "ds.h"
#include "rts_font.h"
#include "asset/inc.h"
#include "asset.h"
#include "animation.h"
#include "cdt.h"
#include "game.h"
#include "rts_entity.h"
#include "geogen.h"
#include "rts_win32.h"
#include "renderer/rts_renderer.h"
#include "rts_win32_renderer.h"
#include "ui/ui_inc.h"
#include "rect_pack/rpk.h"
#include "font_provider/fp_inc.h"
#include "third_party/stb/stb_image.h"

// Globals
//
global Game_State*  game_state;
global Renderer*    renderer;

// .cpp
//
#include "third_party/xxhash3/xxhash.c"
#include "base/rts_base_inc.cpp"
#include "os/os.cpp"
#include "file_system/file.cpp"
#include "rts_random.cpp"
#include "ds.cpp"
#include "rts_font.cpp"
#include "asset/inc.cpp"
#include "asset.cpp"
#include "animation.cpp"
#include "cdt.cpp"
#include "rts_entity.cpp"
#include "renderer/rts_renderer.cpp"
#include "geogen.cpp"
#include "ui/ui_inc.cpp"
#include "rect_pack/rpk.cpp"
#include "font_provider/fp_inc.cpp"

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

#define STBIW_ASSERT(x)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"


#include "temporary.h"


// NOTE: Globals
//
global Win32_State          win32;
global b32                  g_show_cursor = true;

// NOTE: Code Reloading
//
void unload_code(Win32_Code *loaded)
{
    if (loaded->dll)
    {
        // TODO: Currently, we never unload libraries, because we may still be pointing to strings that are inside them
        //        (despite our best efforts). Should we just make "never unload" be the policy?
        // FreeLibrary(loaded->dll);
        loaded->dll = 0;
    }

    loaded->is_valid = false;
    zero_array(loaded->functions, loaded->function_count);
}

void load_code(Win32_Code *loaded)
{
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Utf8 dll_path       = loaded->dll_path;
    Utf8 temp_dll_path  = loaded->temp_dll_path;
    Utf8 lock_path      = loaded->lock_path;

    u64 file_size = os_get_file_size(dll_path);
    if (file_size > 0)
    {
        // load the temporary dll so we could write to the real dll and check the modified time of it.
        loaded->temp_dll_path_prefix = (loaded->temp_dll_path_prefix + 1) % 2;
        temp_dll_path = utf8f(scratch.arena, "%S/%S_%d.dll", win32.binary_path, loaded->temp_dll_name, loaded->temp_dll_path_prefix);
        os_copy_file(temp_dll_path, dll_path);

        Utf16 temp_dll_path16 = to_utf16(scratch.arena, temp_dll_path);
        loaded->dll = LoadLibraryW((WCHAR *)temp_dll_path16.str);

        // if loaded properly, get proc addresses.
        if (loaded->dll)
        {
            loaded->is_valid = true;

            for (u32 i = 0; i < loaded->function_count; ++i)
            {
                void *code = (void *)GetProcAddress(loaded->dll, loaded->function_names[i]);
                if (code) 
                {
                    loaded->functions[i] = code;
                }
                else 
                {
                    loaded->is_valid = false;
                }
            }
        }
    }

    // if libary nor proc isn't loaded, unload the code.
    if (! loaded->is_valid) 
    {
        unload_code(loaded);
    }
}

int main_entry(int argc, char** argv)
{
    // Init core.
    os_init();
    thread_init();
    os_gfx_init();


    // Alloc and init game state.
    {
        Arena* arena = arena_alloc();
        game_state = push_struct(arena, Game_State);
        game_state->arena = arena;

        // Get .exe and data (asset) path.
        {
            Temporary_Arena scratch = scratch_begin();
            defer(scratch_end(scratch));

            Utf8 binary_path = os->binary_path;
            Utf8 data_path = {};

            Utf8 local_data_path = utf8f(scratch.arena, "%S/data", binary_path);
            Utf8 binary_parent_path = utf8_path_chop_last_slash(binary_path);
            Utf8 parent_data_path = utf8f(scratch.arena, "%S/data", binary_parent_path);

            if (os_directory_exists(local_data_path))
                data_path = utf8_copy(arena, local_data_path); 
            else if (os_directory_exists(parent_data_path))
                data_path = utf8_copy(arena, parent_data_path); 

            game_state->binary_path = binary_path;
            game_state->data_path   = data_path;
        }

        // Create main window.
        game_state->main_window = os_create_window(0, 0, 1920, 1080, utf8lit("rts"));
    }

    // Init win32 state.
    // 
    {
        win32.arena = arena_alloc();

        Utf8 binary_path = game_state->binary_path;

        win32.binary_path        = binary_path;
        win32.renderer_dll_path  = utf8f(win32.arena, "%S/rts_renderer_opengl.dll", binary_path);
        win32.lock_path          = utf8f(win32.arena, "%S/lock.tmp", binary_path);
    }


    // Alloc and init renderer.
    // 
    {
        Arena* arena = arena_alloc();
        renderer = push_struct(arena, Renderer);
        renderer->arena = arena;
        render_init();
    }


    HWND hwnd = hwnd_from_os_handle(game_state->main_window);
    HDC renderer_hdc = GetDC(hwnd);
    b32 renderer_was_reloaded = false;
    Win32_Renderer_Function_Table renderer_functions = {};
    Win32_Code renderer_code = {};
    {
        renderer_code.temp_dll_name  = utf8lit("renderer_temp");
        renderer_code.temp_dll_path  = utf8f(win32.arena, "%S/%S.dll", win32.binary_path, renderer_code.temp_dll_name);
        renderer_code.dll_path       = win32.renderer_dll_path;
        renderer_code.lock_path      = win32.lock_path;
        renderer_code.function_count = array_count(win32_renderer_function_table_names);
        renderer_code.functions      = (void **)&renderer_functions;
        renderer_code.function_names = win32_renderer_function_table_names;
    }
    load_code(&renderer_code);
    if (!renderer_code.is_valid) {
        assert(!"Couldn't load the renderer code."); 
    }

    Arena* renderer_arena = arena_alloc();
    Platform_Renderer* platform_renderer = renderer_functions.load_renderer(renderer_hdc, MB(50), renderer_arena, os);


    // Main Loop
    //
    u64 old_counter = os_counter();
    while (!game_state->should_close) 
    {
        // Draw resolution.
        v2 resolution = {
            //960, 540
            //1280, 720
            //1920, 1080,
            2560, 1440,
        };

        // Clear and poll events.
        os_clear_events();
        os_poll_events();

        // Get delta-time.
        u64 new_counter = os_counter();
        f32 actual_dt = (new_counter - old_counter) * os_counter_freq_rcp();
        old_counter = new_counter;

        // Get window and render size.
        v2 window_size    = os_get_window_size(game_state->main_window);
        u32 window_width  = window_size.x;
        u32 window_height = window_size.y;

        Render_Commands *render_commands = NULL;

        // Render begin.
        //
        if (renderer_code.is_valid) {
            render_commands = renderer_functions.begin_frame(platform_renderer, window_size, resolution); 
        }

        // Game
        //
        {
            ProfileFrameMark;
            ProfileScope;

            using namespace Asset;

            // @Temporary
            const f32 tick_dt = 1.f / 60.f;
            local_persist f32 game_speed = 1.f;
            f32 dt = actual_dt * game_speed;

            // Update draw/window dimension.
            // @Todo: Switch to immediate-mode?
            game_state->draw_width    = resolution.x;
            game_state->draw_height   = resolution.y;
            game_state->window_width  = window_width;
            game_state->window_height = window_height;

            if (!game_state->initted) {
                game_state->initted = true;

                thread_init();

                // Alloc assets
                //
                Arena *arena = arena_alloc();
                game_state->assets = push_struct(arena, Game_Assets);
                game_state->assets->arena = arena;

                game_state->frame_arena = arena_alloc();

                game_state->random_series = rand_seed(1219);

                game_state->entity_arena    = arena_alloc();
                game_state->animation_arena = arena_alloc();

                game_state->num_skinning_matrices = 0;
                game_state->max_skinning_matrices = RHI::max_num_skinning_matrices;
                //game_state->skinning_matrices = push_array(game_state->animation_arena, m4x4, game_state->max_skinning_matrices);
                game_state->skinning_matrices = render_commands->skinning_matrices;

                game_state->root_entity          = push_struct(game_state->entity_arena, Entity);
                game_state->root_entity->type    = ENTITY_TYPE_ROOT;
                game_state->entity_table_size    = 1024; // FIX: Memory bug in arena when set size to 4096
                game_state->entity_table         = push_array(game_state->entity_arena, Entity, game_state->entity_table_size);
                game_state->next_generational_id = 1;

                game_state->minimap_size         = 300.f;

                Asset::init(&game_state->asset_system);

                { // @Temporary
                    Temporary_Arena scratch = scratch_begin();
                    defer(scratch_end(scratch));

                    Game_Assets *assets = game_state->assets;
                    Arena *asset_arena = game_state->assets->arena;
                    auto asset_system = &game_state->asset_system;

                    assets->skeleton_model = push_struct(asset_arena, Model);
                    {
                        auto *model = assets->skeleton_model;
                        load_model(asset_arena, model, utf8f(scratch.arena, "%S/mesh/skeleton.triangle_mesh", game_state->data_path));

                        {
                            auto *mesh = mesh_from_name(model, utf8lit("body-lib"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/skeleton_body.material", game_state->data_path));
                        }

                        {
                            auto *mesh = mesh_from_name(model, utf8lit("helm-lib"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/skeleton_helm.material", game_state->data_path));
                        }

                        {
                            auto *mesh = mesh_from_name(model, utf8lit("scabbard_2-lib"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/skeleton_scabbard.material", game_state->data_path));
                        }



                        assets->skeleton_idle = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_idle, utf8f(scratch.arena, "%S/animation/skeleton_lord_idle.keyframed_animation", game_state->data_path));

                        assets->skeleton_run = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_run, utf8f(scratch.arena, "%S/animation/skeleton_lord_run.keyframed_animation", game_state->data_path));

                        assets->skeleton_attack = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_attack, utf8f(scratch.arena, "%S/animation/skeleton_lord_attack.keyframed_animation", game_state->data_path));

                        assets->skeleton_die = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_die, utf8f(scratch.arena, "%S/animation/skeleton_lord_die.keyframed_animation", game_state->data_path));
                    }

                    assets->skeleton_skeleton = push_struct(asset_arena, Skeleton);
                    load_skeleton(asset_arena, assets->skeleton_skeleton, utf8f(scratch.arena, "%S/skeleton/skeleton_lord.skeleton", game_state->data_path));  


                    // Knight
                    assets->knight_model = push_struct(asset_arena, Model);
                    {
                        // Model
                        auto *model = assets->knight_model;
                        load_model(asset_arena, model, utf8f(scratch.arena, "%S/mesh/knight.triangle_mesh", game_state->data_path));

                        // Skeleton
                        assets->knight_skeleton = push_struct(asset_arena, Skeleton);
                        load_skeleton(asset_arena, assets->knight_skeleton, utf8f(scratch.arena, "%S/skeleton/knight.skeleton", game_state->data_path));  

                        // Animations
                        assets->knight_idle = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_idle, utf8f(scratch.arena, "%S/animation/knight_idle.keyframed_animation", game_state->data_path));

                        assets->knight_run = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_run, utf8f(scratch.arena, "%S/animation/knight_run.keyframed_animation", game_state->data_path));

                        assets->knight_attack = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_attack, utf8f(scratch.arena, "%S/animation/knight_attack.keyframed_animation", game_state->data_path));

                        assets->knight_die = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_die, utf8f(scratch.arena, "%S/animation/knight_die.keyframed_animation", game_state->data_path));

                        // @Todo: How do we cope with material that's already loaded? Hash table. I need a solid hash table...
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Helm2"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/helm.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Arms"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/arms.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Acessories"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/arms.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Acessories2"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/arms.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Breast_Armor"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/breast_armor.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Leegs_Armor1"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/breast_armor.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("pants"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/breast_armor.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Weapon2"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/sword.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Shield"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, utf8f(scratch.arena, "%S/materials/knight/shield.material", game_state->data_path));
                        }
                    }

                    assets->plane_model = push_struct(asset_arena, Model);
                    {
                        // @Temporary: Scaled 100x, because the exported mesh from Maya is in centimeter atm.
                        load_model(asset_arena, assets->plane_model, utf8f(scratch.arena, "%S/mesh/plane_256.triangle_mesh", game_state->data_path), v3(100.f));

                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_ALBEDO], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_albedo.texture", game_state->data_path));
                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_NORMAL], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_normal-ogl.texture", game_state->data_path));
                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_ROUGHNESS], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_roughness.texture", game_state->data_path));
                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_METALLIC], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_metallic.texture", game_state->data_path));
                    }

                    assets->sword_model = push_struct(asset_arena, Model);
                    {
                        auto* model = assets->sword_model;
                        load_model(asset_arena, assets->sword_model, utf8f(scratch.arena, "%S/mesh/sword.triangle_mesh", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_ALBEDO], utf8f(scratch.arena, "%S/textures/sword_albedo.texture", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_NORMAL], utf8f(scratch.arena, "%S/textures/sword_normal.texture", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_METALLIC], utf8f(scratch.arena, "%S/textures/sword_metalic.texture", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_ROUGHNESS], utf8f(scratch.arena, "%S/textures/sword_roughness.texture", game_state->data_path));
                    }

                    assets->castle_model = push_struct(asset_arena, Model);
                    {
                        auto* model = assets->castle_model;
                        load_model(asset_arena, model, utf8f(scratch.arena, "%S/mesh/castle.triangle_mesh", game_state->data_path), v3(8.f));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_ALBEDO], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_albedo.texture", game_state->data_path));
                    }

                    char *skybox_filenames[6] = {"right", "left", "top", "bottom", "front", "back"};
                    for (u32 i = 0; i < 6; ++i) {
                        load_texture(asset_system, assets->skybox_textures + i, utf8f(scratch.arena, "%S/textures/%s.texture", game_state->data_path, skybox_filenames[i]));
                    }

                    load_texture(asset_system, &assets->height_map, utf8f(scratch.arena, "%S/textures/height_map.texture", game_state->data_path));

                    game_state->map_arena     = arena_alloc();
                    game_state->chunk_size    = v2(3.f, 3.f);
                    game_state->chunk_count_x = 128;
                    game_state->chunk_count_y = 128;
                    game_state->map_size.x    = game_state->chunk_size.x * game_state->chunk_count_x;
                    game_state->map_size.y    = game_state->chunk_size.y * game_state->chunk_count_y;
                    game_state->chunks        = push_array(game_state->map_arena, Chunk, game_state->chunk_count_x * game_state->chunk_count_y);

                    {
                        Entity *game_camera       = entity_alloc();
                        game_camera->type         = ENTITY_TYPE_CAMERA;
                        //game_camera->focal_length = 1.6f;
                        game_camera->focal_length = 1.1f;
                        game_camera->N            = 0.5f;
                        game_camera->F            = 100000.0f;
                        game_camera->position     = v3(0.f, 18.f, 20.f);
                        game_camera->orientation  = euler_to_quaternion(radian_from_degree(-50.f), 0.f, 0.f);
                        game_camera->flags |= ENTITY_FLAG_GAME_CAMERA;
                        entity_init(game_camera, nullptr);

                        Entity *debug_camera       = entity_alloc();
                        debug_camera->type         = ENTITY_TYPE_CAMERA;
                        debug_camera->focal_length = 0.5f;
                        debug_camera->N            = 0.5f;
                        debug_camera->F            = 100000.0f;
                        debug_camera->position     = v3(0.f,5.f,5.f);
                        debug_camera->orientation  = euler_to_quaternion(radian_from_degree(-45.f), 0.f, 0.f);
                        debug_camera->flags |= ENTITY_FLAG_FREE_CAMERA;
                        entity_init(debug_camera, nullptr);

                        game_state->game_camera_id        = game_camera->id;
                        game_state->debug_camera_id       = debug_camera->id;
                        game_state->controlling_camera_id = game_camera->id;
                    }


                    geogen_backfaced_cube(&assets->skybox_mesh, asset_arena, 10000);


                    render_commands->csm_varient_method = true;

                    // Initialize CDT context.
                    //
                    f32 hx = 0.5f*game_state->map_size.x;
                    f32 hy = 0.5f*game_state->map_size.y;
                    cdt_init(&game_state->navmesh.ctx, 0.f, 2048.f, -2048.f, -2048.f, 2048.f, -2048.f); // @Temporary


                    // Push map boundary to the navmesh.
                    //
                    cdt_insert(&game_state->navmesh.ctx, 0, -hx, hy, -hx, -hy);
                    cdt_insert(&game_state->navmesh.ctx, 0, -hx,-hy,  hx, -hy);
                    cdt_insert(&game_state->navmesh.ctx, 0,  hx,-hy,  hx,  hy);
                    cdt_insert(&game_state->navmesh.ctx, 0,  hx, hy, -hx,  hy);


                    // @Temporary: Create soldier entity.
                    //
                    int num_soldiers = 32;
                    int num_rows = 50;
                    f32 dist = 0.8f;

                    for (int i = 0; i < num_soldiers; ++i) 
                    {
                        f32 x = 6.f + dist*(i / num_rows);
                        f32 z = 0.f + dist*(i % num_rows);
#if 1
                        Entity* soldier = debug_spawn_knight(x, z, TEAM_PLAYER, assets);
#else
                        Entity* soldier = debug_spawn_soldier(x, z, TEAM_PLAYER, assets); 
                        debug_attach_sword(soldier, assets);
#endif
                    }

#if 1
                    for (int i = 0; i < num_soldiers; ++i) {
                        f32 x = 30.f + dist*(i / num_rows);
                        f32 z =  0.f + dist*(i % num_rows);
                        Entity* soldier = debug_spawn_soldier(x, z, TEAM_ENEMY, assets);

                        debug_attach_sword(soldier, assets);
                    }
#endif

                    debug_spawn_castle( 0.f,  0.f, TEAM_PLAYER, assets);
                    //debug_spawn_castle( 0.f, -8.f, TEAM_PLAYER, assets);
                }
            }



            arena_clear(game_state->frame_arena);
            render_begin();

            if (fp_state == NULL) {
                fp_state = fp_alloc();
                fp_init();
            }

            if (ui_state == NULL) {
                ui_state = ui_alloc();
                ui_init(ui_state);
            }

            // Alias
            //
            Game_Assets *assets = game_state->assets;
            auto asset_system = &game_state->asset_system;

            Render_Group* render_group = begin_render_group(render_commands, MB(16));
            v2 current_mouse_position = os_get_mouse_position(game_state->main_window);


            // Get all triangles in the navmesh
            //
            int triangle_count = cdt_get_triangle_count(&game_state->navmesh.ctx);
            game_state->navmesh.triangles = push_array(game_state->frame_arena, cdt_triangle, triangle_count);
            cdt_get_all_triangles(&game_state->navmesh.ctx, game_state->navmesh.triangles);
            game_state->navmesh.triangle_count = triangle_count;



            // @Temporary: Testing UI
            //             1. Interact with UI built in last frame.
            //             2. Build new hierarchy while retaining some data(!!!)
            //
            local_persist f32 light_x = 1.f, light_y = 1.f, light_z = 1.f;
            local_persist b32 draw_chunk_partitions = false;
#if 1
            ui_begin(actual_dt, window_width, window_height, os_get_mouse_position(game_state->main_window));
            {
#if BUILD_DEBUG
                ui_platform(utf8lit("Debug Build"))
#else
                    ui_platform(utf8lit("Release Build"))
#endif
                    {
                        f32 mspf = actual_dt * 1000.f;
                        f32 fps = 1.f / actual_dt;
                        ui_labelf("mspf: %.2f | fps: %.2f", mspf, fps);
                        ui_slider_f32(&ui_state->font_size, 8.f, 30.f, utf8lit("Font Size"));
                        if (ui_button(utf8lit("Chunk Partitions")).pressed_left) {
                            draw_chunk_partitions = !draw_chunk_partitions;
                        }
                        if (ui_button(utf8lit("Show Chunk Position")).pressed_left) {
                            game_state->display_chunk_position = !game_state->display_chunk_position;
                        }
                        if (ui_button(utf8lit("Wireframe")).pressed_left) {
                            render_commands->wireframe_mode = !render_commands->wireframe_mode; 
                        }
                        if (ui_button(utf8lit("Navmesh")).pressed_left) {
                            render_commands->draw_navmesh = !render_commands->draw_navmesh; 
                        }

                        ui_slider_f32(&game_speed, 0.25f, 3.f, utf8lit("Game Speed"));
                        ui_slider_f32(&game_state->minimap_size, 1.f, 1000.f, utf8lit("Minimap Size"));

                        if (ui_expander(utf8lit("Shadow"))) {
                            ui_slider_f32(&light_x, -1.0f, 1.0f, utf8lit("x"));
                            ui_slider_f32(&light_y, -1.0f, 1.0f, utf8lit("y"));
                            ui_slider_f32(&light_z, -1.0f, 1.0f, utf8lit("z"));
                            if (ui_button(utf8lit("Valient's Method")).pressed_left) {
                                render_commands->csm_varient_method = !render_commands->csm_varient_method; 
                            }
                            if (ui_button(utf8lit("CSM Frustum")).pressed_left) {
                                render_commands->draw_csm_frustum = !render_commands->draw_csm_frustum; 
                            }
                        }
                        if (ui_expander(utf8lit("Camera"))) {
                            if (ui_button(utf8lit("Switch Camera")).pressed_left) {
                                if (game_state->controlling_camera_id == game_state->game_camera_id) {
                                    game_state->controlling_camera_id = game_state->debug_camera_id;
                                } else {
                                    game_state->controlling_camera_id = game_state->game_camera_id;
                                }
                            }
                            ui_slider_f32(&entity_from_id(game_state->controlling_camera_id)->focal_length, 0.001f, 10.0f, utf8lit("Focal Length"));
                        }
                    }
            }
            ui_end();
#endif

            //
            // Entity selection.
            //
            if (game_state->controlling_camera_id == game_state->game_camera_id) {
                Temporary_Arena scratch = scratch_begin();
                defer(scratch_end(scratch));

                local_persist bool dragging = false;
                local_persist v2   drag_start = {};

                list_for (os->first_event, event)
                {
                    if (event->key == KEY_MOUSE_LEFT) {

                        if (event->kind == OS_EVENT_PRESS) {
                            dragging = true;
                            os_remove_event(event);
                            drag_start = current_mouse_position;
                        }

                        if (dragging && event->kind == OS_EVENT_RELEASE) {
                            dragging = false;
                            os_remove_event(event);

                            // get entities
                            Entity* camera = entity_from_id(game_state->game_camera_id);
                            const m4x4 viewproj = camera->VP;
                            const f32 w = (f32)game_state->window_width;
                            const f32 h = (f32)game_state->window_height;

                            const f32 min_screen_x = min(drag_start.x, current_mouse_position.x);
                            const f32 min_screen_y = min(drag_start.y, current_mouse_position.y);
                            const f32 max_screen_x = max(drag_start.x, current_mouse_position.x);
                            const f32 max_screen_y = max(drag_start.y, current_mouse_position.y);

                            Ray3 ray1 = ray_from_screen_position(drag_start, w, h, viewproj);
                            Ray3 ray2 = ray_from_screen_position(current_mouse_position, w, h, viewproj);
                            const  v3 n = v3{0,1,0};
                            const f32 d = 0.f;
                            v3 p1 = {};
                            v3 p2 = {};
                            const bool intersects = (ray_plane_intersect(ray1, n, d, &p1) && ray_plane_intersect(ray2, n, d, &p2));
                            if (intersects) {
                                const f32 min_x = min(p1.x, p2.x) - game_state->max_radius;
                                const f32 min_z = min(p1.z, p2.z) - game_state->max_radius;
                                const f32 max_x = max(p1.x, p2.x) + game_state->max_radius;
                                const f32 max_z = max(p1.z, p2.z) + game_state->max_radius;
                                u16 min_chunk_x, min_chunk_y, max_chunk_x, max_chunk_y;
                                chunk_position_from_world_position(min_x, min_z, &min_chunk_x, &min_chunk_y); 
                                chunk_position_from_world_position(max_x, max_z, &max_chunk_x, &max_chunk_y); 

                                List <Entity*> entities = entities_from_min_max_chunk(scratch.arena, min_chunk_x, min_chunk_y, max_chunk_x, max_chunk_y);
                                if (!entities.is_empty()) {

                                    // 'selected' flag from entities and clear the list.
                                    for (auto node = game_state->selected_entities.first; node; node = node->next) {
                                        u64 id = node->data;
                                        Entity* entity = entity_from_id(id);
                                        if (entity) {
                                            entity->flags &= (~ENTITY_FLAG_SELECTED);
                                        }
                                    }
                                    game_state->selected_entities.clear();


                                    // Fill and set 'selected' flag.
                                    for (auto node = entities.first; node; node = node->next) {
                                        Entity* entity = node->data;

                                        if (!entity) {
                                            continue;
                                        }

                                        if (entity_is_dead(entity)) {
                                            continue;
                                        }

                                        if (entity->team != TEAM_PLAYER) {
                                            continue;
                                        }

                                        const v3 ndc = project(entity->position, camera->VP);
                                        const f32 x = ( ndc.x * 0.5f + 0.5f) * w;
                                        const f32 y = (-ndc.y * 0.5f + 0.5f) * h;
                                        if (x >= min_screen_x && x <= max_screen_x && y >= min_screen_y && y <= max_screen_y) {
                                            game_state->selected_entities.add(entity->id);
                                            entity->flags |= ENTITY_FLAG_SELECTED;
                                        }
                                    }
                                }
                            } else {
                                assert(!"No intersection? Seems weird.");
                            }
                        }
                    }
                }

                if (dragging) {
                    const f32 w = (f32)game_state->window_width;
                    const f32 h = (f32)game_state->window_height;

                    const v4 color = v4{1.0f, 1.0f, 1.0f, 0.2f};
                    const f32 thickness = 1.f;
                    const f32 min_x = min(drag_start.x, current_mouse_position.x);
                    const f32 min_y = min(drag_start.y, current_mouse_position.y);
                    const f32 max_x = max(drag_start.x, current_mouse_position.x);
                    const f32 max_y = max(drag_start.y, current_mouse_position.y);
                    render_quad_c(v2{min_x - thickness, min_y - thickness}, v2{max_x + thickness, min_y}, color);
                    render_quad_c(v2{max_x, min_y}, v2{max_x + thickness, max_y}, color);
                    render_quad_c(v2{min_x - thickness, min_y}, v2{min_x, max_y}, color);
                    render_quad_c(v2{min_x - thickness, max_y}, v2{max_x + thickness, max_y + thickness}, color);
                }
            }


            // Draw chunks
            //
            if (draw_chunk_partitions) {
                const f32 half_dim_x = 0.5f * game_state->chunk_count_x * game_state->chunk_size.x;
                const f32 half_dim_y = 0.5f * game_state->chunk_count_y * game_state->chunk_size.y;

                const f32 alpha = 0.7f;

                for (int cy = 0; cy < game_state->chunk_count_y; ++cy) {
                    const f32 y = -half_dim_y + game_state->chunk_size.y * cy;
                    draw_line(render_group, v3{-half_dim_x,0.2f,y}, v3{half_dim_x,0.0f,y}, v4{1.f,0.3f,0.3f,alpha});
                }

                for (int cx = 0; cx < game_state->chunk_count_x; ++cx) {
                    const f32 x = -half_dim_x + game_state->chunk_size.x * cx;
                    draw_line(render_group, v3{x,0.2f,-half_dim_y}, v3{x,0.0f,half_dim_y}, v4{0.3f,0.3f,1.0f,alpha});
                }
            }


            // Update animation players
            //
            {
                ProfileScopeNC("update animation players", 0xffc5d3);

                list_for(game_state->first_animation_player, ap) {
                    Update_Animation_Param *param = push_struct(game_state->frame_arena, Update_Animation_Param);
                    {
                        param->ap = ap;
                        param->dt = dt;
                    }
                    add_work(&os->work_queue, update_animation_player_work, param);
                }
                complete_all_work(&os->work_queue);
            }


            // Update entities
            //
            entity_update(game_state->root_entity, dt);


            // Draw entities
            //
            entity_draw(game_state->root_entity, dt, render_group, render_commands);



            // Draw ground
            //
            Mesh* ground_mesh = assets->plane_model->meshes;
            f32 gx = game_state->map_size.x;
            f32 gy = game_state->map_size.y;
            m4x4 ground_transform = m4x4_scale(gx, 1.f, gy);
            v2 uv_scale = v2(gx, gy) * 0.1f;
            push_mesh(renderer, ground_mesh, ground_transform, 0, 0, uv_scale);


            // @Temporary: Draw mesh
            //
            Mesh* water = assets->plane_model->meshes;


            // Draw navmesh
            //
            if (render_commands->draw_navmesh) {
                Entity *controlling_camera = entity_from_id(game_state->controlling_camera_id);
                m4x4 view_proj = controlling_camera->VP;
                for (int i = 0; i < game_state->navmesh.ctx.edges.num; ++i) {
                    cdt_edge *edge = game_state->navmesh.ctx.edges.data[i];
                    cdt_quad_edge *qe = &edge->e[0];
                    v2 a = qe->org->pos;
                    v2 b = cdt_sym(qe)->org->pos;
                    v4 color = cdt_is_constrained(edge) ? V4(1.f, 0.f, 1.f, 1.f) : V4(1.f, 1.f, 1.f, 1.f);
                    draw_line(render_group, v3(a.y, 0.f, a.x), v3(b.y, 0.f, b.x), color);
                }
            }

            //
            // Draw minimap.
            // @Todo: aspect ratio adjustment..
            //
            {
                auto g = game_state;

                // Draw bordered quad.
                //
                f32 dim         = g->minimap_size;
                v2 offset       = v2(30.f, 30.f);
                v2 bottom_left  = v2(offset.x, g->window_height - offset.y);
                v2 top_left     = v2(bottom_left.x, bottom_left.y - dim);
                v2 bottom_right = v2(bottom_left.x + dim, bottom_left.y);
                v2 border       = v2(2.f, 2.f);
                render_quad_c(top_left - border, bottom_right + border, v4{0.0f, 0.0f, 0.0f, 1.f});
                render_quad_c(top_left, bottom_right, v4{0.2f, 0.2f, 0.2f, 1.f});

                // Draw camera.
                //

                // Draw entities on the minimap.
                //
                for (u32 i = 0; i < g->entity_table_size; ++i) {
                    Entity* bucket = g->entity_table + i;
                    for (Entity* entity = bucket->first; entity; entity = entity->next_in_table) {
                        if (entity->flags & ENTITY_FLAG_SHOWS_ON_MINIMAP) {

                            if (entity_is_dead(entity)) {
                                continue;
                            }

                            v3 position = entity->position;
                            f32 nx = map(position.x, -0.5f*g->map_size.x, 0.5f*g->map_size.x);
                            f32 ny = map(position.z, -0.5f*g->map_size.y, 0.5f*g->map_size.y);

                            f32 x  = top_left.x + dim*nx;
                            f32 y  = top_left.y + dim*ny;
                            f32 hd = 2.f;

                            v4 color = v4{0.3f, 1.f, 0.3f, 1.f};
                            if (entity->team != TEAM_PLAYER) {
                                color = v4{1.0f, 0.3f, 0.3f, 1.f};
                            }

                            v2 unit_border = v2(1.f,1.f);
                            render_quad_c(v2(x - hd, y - hd) - unit_border, v2(x + hd, y + hd) + unit_border, v4{0.1f, 0.1f, 0.1f, 1.f});
                            render_quad_c(v2(x - hd, y - hd), v2(x + hd, y + hd), color);
                        }
                    }
                }
            }


            { // Render Commands
                Entity* game_camera = entity_from_id(game_state->game_camera_id);
                Entity* controlling_camera = entity_from_id(game_state->controlling_camera_id);

                render_commands->main_eye_position = controlling_camera->position;
                render_commands->main_view_proj    = controlling_camera->VP;

                render_commands->wireframe_color = V4(0.9f, 0.9f, 0.9f, 1.0f);

                // Skybox
                //
                render_commands->skybox_mesh = &assets->skybox_mesh;
                render_commands->skybox_eye_view_proj = controlling_camera->VP;
                render_commands->skybox_textures = assets->skybox_textures;


                // CSM
                //
                render_commands->csm_to_light = normalize(v3(light_x, light_y, light_z));
                f32 csm_frustum_edge_length = 300.0f;
                m4x4 inv = inverse(game_camera->VP);
                // @Todo: Renderer independent calculation!
                v4 ndcs[4] = {
                    v4{-1,-1,-1, 1},
                    v4{ 1,-1,-1, 1},
                    v4{-1, 1,-1, 1},
                    v4{ 1, 1,-1, 1},
                };

                v3 eye = game_camera->position;
                v4 positions[8];

                for (u32 i = 0; i < 4; ++i) {
                    positions[i] = inv * ndcs[i];
                    positions[i].xyz *= (1.f / positions[i].w);
                }

                for (u32 i = 0; i < 4; ++i) {
                    v3 d = normalize(positions[i].xyz - eye);
                    positions[4+i] = positions[i];
                    positions[4+i].xyz += (csm_frustum_edge_length*d);
                }

                for (u32 i = 0; i < 8; ++i) {
                    render_commands->csm_frustum_positions[i] = positions[i].xyz;
                }
                render_commands->csm_view = game_camera->V;
            }

            render_end();
        }

        // Render Pass
        //
        if (renderer_code.is_valid) {
            renderer_functions.end_frame(platform_renderer, renderer, render_commands);
        }

        // Close?
        list_for(os->first_event, event) {
            // Alt + F4?
            if (event->kind == OS_EVENT_PRESS && event->key == KEY_F4 && (event->modifiers & OS_MODIFIER_ALT)) {
                game_state->should_close = true;
                os_remove_event(event);
            } 
            // Close main window triggered?
            if (event->kind == OS_EVENT_WINDOW_CLOSE && event->window == game_state->main_window) {
                game_state->should_close = true;
                os_remove_event(event);
            }
        }
    }

    return 0;
}
