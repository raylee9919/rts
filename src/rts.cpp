// Copyright Seong Woo Lee. All Rights Reserved.

// .h
//
#include "Profiler/profiler.h"

#include "Basic/include.h"
#include "Math/include.h"
#include "OS/include.h"
#include "thread/includes.h"
#include "ds.h"
#include "rts_font.h"
#include "asset/inc.h"
#include "asset.h"
#include "animation/animation.h"
#include "cdt.h"
#include "rts.h"
#include "rts_entity.h"
#include "geogen.h"
#include "renderer/rts_renderer.h"
#include "ui/ui_inc.h"
#include "RectPack/include.h"
#include "font_provider/fp_inc.h"
#include "RHI/include.h"

#include "ThirdParty/stb/stb_image.h"
#include "ThirdParty/stb/stb_image_write.h"


// Globals
//
global Game_State *game_state;
global Renderer   *renderer;
global RHI_State  *g_rhi_state;

// .cpp
//
#include "Profiler/profiler.h"
#include "ThirdParty/xxhash3/xxhash.c"

#include "Basic/include.cpp"
#include "Math/include.cpp"
#include "OS/include.cpp"
#include "thread/includes.cpp"
#include "ds.cpp"
#include "rts_font.cpp"
#include "asset/inc.cpp"
#include "asset.cpp"
#include "animation/animation.cpp"
#include "cdt.cpp"
#include "rts_entity.cpp"
#include "renderer/rts_renderer.cpp"
#include "geogen.cpp"
#include "ui/ui_inc.cpp"
#include "RectPack/include.cpp"
#include "font_provider/fp_inc.cpp"
#include "RHI/include.cpp"

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image.h"

#define STBIW_ASSERT(x)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ThirdParty/stb/stb_image_write.h"


#include "temporary.h"


int main_entry(int argc, char **argv)
{
    // Alloc and init game state.
    {
        Arena* arena = arena_alloc();
        game_state = push_struct(arena, Game_State);
        game_state->arena = arena;

        // Get .exe and data (asset) path.
        {
            String binary_path = os->binary_path;
            String data_path = {};

            String local_data_path    = tprint("%S/data", binary_path);
            String binary_parent_path = utf8_path_chop_last_slash(binary_path);
            String parent_data_path   = tprint("%S/data", binary_parent_path);

            if (os_directory_exists(local_data_path)) {
                data_path = utf8_copy(arena, local_data_path); 
            } else if (os_directory_exists(parent_data_path)) {
                data_path = utf8_copy(arena, parent_data_path); 
            }

            game_state->binary_path = binary_path;
            game_state->data_path   = data_path;
        }

        // Create main window.
        game_state->main_window = os_window_create(1920, 1080, utf8lit("rts"));
    }

    // Alloc and init RHI.
    {
        Arena* arena = arena_alloc();
        g_rhi_state = push_struct(arena, RHI_State);
        g_rhi_state->arena = arena;
        rhi_init(g_rhi_state, game_state->main_window);
    }


    // Alloc and init renderer.
    {
        Arena* arena = arena_alloc();
        renderer = push_struct(arena, Renderer);
        renderer->arena = arena;
        render_init();
    }


    // Main Loop
    //
    u64 old_counter = os_counter();
    for (bool should_close = false; !should_close;)
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
        v2 window_size    = os_window_size(game_state->main_window);
        u32 window_width  = window_size.x;
        u32 window_height = window_size.y;

        // Render begin.
        //
        r_begin(g_rhi_state, window_size, resolution);
        auto *render_commands = get_render_commands(g_rhi_state);


        // Game
        //
        {
            using namespace Asset;

            // @Temporary
            f32 tick_dt = 1.f / 60.f;
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

                // Alloc assets
                //
                Arena *arena = arena_alloc();
                game_state->assets = push_struct(arena, Game_Assets);
                game_state->assets->arena = arena;

                game_state->frame_arena = arena_alloc();

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
                        load_model(asset_arena, model, tprint("%S/mesh/skeleton.triangle_mesh", game_state->data_path));
                        load_model(asset_arena, model, tprint("%S/mesh/skeleton.triangle_mesh", game_state->data_path));

                        {
                            auto *mesh = mesh_from_name(model, utf8lit("body-lib"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/skeleton_body.material", game_state->data_path));
                        }

                        {
                            auto *mesh = mesh_from_name(model, utf8lit("helm-lib"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/skeleton_helm.material", game_state->data_path));
                        }

                        {
                            auto *mesh = mesh_from_name(model, utf8lit("scabbard_2-lib"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/skeleton_scabbard.material", game_state->data_path));
                        }



                        assets->skeleton_idle = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_idle, tprint("%S/animation/skeleton_lord_idle.keyframed_animation", game_state->data_path));

                        assets->skeleton_run = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_run, tprint("%S/animation/skeleton_lord_run.keyframed_animation", game_state->data_path));

                        assets->skeleton_attack = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_attack, tprint("%S/animation/skeleton_lord_attack.keyframed_animation", game_state->data_path));

                        assets->skeleton_die = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->skeleton_die, tprint("%S/animation/skeleton_lord_die.keyframed_animation", game_state->data_path));
                    }

                    assets->skeleton_skeleton = push_struct(asset_arena, Skeleton);
                    load_skeleton(asset_arena, assets->skeleton_skeleton, tprint("%S/skeleton/skeleton_lord.skeleton", game_state->data_path));  


                    // Knight
                    assets->knight_model = push_struct(asset_arena, Model);
                    {
                        // Model
                        auto *model = assets->knight_model;
                        load_model(asset_arena, model, tprint("%S/mesh/knight.triangle_mesh", game_state->data_path));

                        // Skeleton
                        assets->knight_skeleton = push_struct(asset_arena, Skeleton);
                        load_skeleton(asset_arena, assets->knight_skeleton, tprint("%S/skeleton/knight.skeleton", game_state->data_path));  

                        // Animations
                        assets->knight_idle = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_idle, tprint("%S/animation/knight_idle.keyframed_animation", game_state->data_path));

                        assets->knight_run = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_run, tprint("%S/animation/knight_run.keyframed_animation", game_state->data_path));

                        assets->knight_attack = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_attack, tprint("%S/animation/knight_attack.keyframed_animation", game_state->data_path));

                        assets->knight_die = push_struct(asset_arena, Animation);
                        load_animation(asset_arena, assets->knight_die, tprint("%S/animation/knight_die.keyframed_animation", game_state->data_path));

                        // @Todo: How do we handle material that's already loaded? Hash table. I need a solid hash table...
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Helm2"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/helm.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Arms"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/arms.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Acessories"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/arms.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Acessories2"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/arms.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Breast_Armor"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/breast_armor.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Leegs_Armor1"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/breast_armor.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("pants"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/breast_armor.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Weapon2"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/sword.material", game_state->data_path));
                        }
                        {
                            auto *mesh = mesh_from_name(model, utf8lit("Shield"));
                            assert(mesh);
                            mesh->material = load_material(asset_system, game_state->data_path, tprint("%S/materials/knight/shield.material", game_state->data_path));
                        }
                    }

                    assets->plane_model = push_struct(asset_arena, Model);
                    {
                        // @Temporary: Scaled 100x, because the exported mesh from Maya is in centimeter atm.
                        load_model(asset_arena, assets->plane_model, tprint("%S/mesh/plane_256.triangle_mesh", game_state->data_path), v3(100.f));

                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_ALBEDO], tprint("%S/textures/wispy-grass-meadow_albedo.texture", game_state->data_path));
                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_NORMAL], tprint("%S/textures/wispy-grass-meadow_normal-ogl.texture", game_state->data_path));
                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_ROUGHNESS], tprint("%S/textures/wispy-grass-meadow_roughness.texture", game_state->data_path));
                        load_texture(asset_system, &assets->plane_model->meshes[0].material.textures[PBR_METALLIC], tprint("%S/textures/wispy-grass-meadow_metallic.texture", game_state->data_path));
                    }

                    assets->sword_model = push_struct(asset_arena, Model);
                    {
                        auto* model = assets->sword_model;
                        load_model(asset_arena, assets->sword_model, tprint("%S/mesh/sword.triangle_mesh", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_ALBEDO], tprint("%S/textures/sword_albedo.texture", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_NORMAL], tprint("%S/textures/sword_normal.texture", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_METALLIC], tprint("%S/textures/sword_metalic.texture", game_state->data_path));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_ROUGHNESS], tprint("%S/textures/sword_roughness.texture", game_state->data_path));
                    }

                    assets->castle_model = push_struct(asset_arena, Model);
                    {
                        auto* model = assets->castle_model;
                        load_model(asset_arena, model, tprint("%S/mesh/castle.triangle_mesh", game_state->data_path), v3(8.f));
                        load_texture(asset_system, &model->meshes[0].material.textures[PBR_ALBEDO], tprint("%S/textures/wispy-grass-meadow_albedo.texture", game_state->data_path));
                    }

                    char *skybox_filenames[6] = {"right", "left", "top", "bottom", "front", "back"};
                    for (u32 i = 0; i < 6; ++i) {
                        load_texture(asset_system, assets->skybox_textures + i, tprint("%S/textures/%s.texture", game_state->data_path, skybox_filenames[i]));
                    }

                    load_texture(asset_system, &assets->height_map, tprint("%S/textures/height_map.texture", game_state->data_path));

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
                        game_camera->position     = v3(0.f, 15.f, 20.f);
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
                    int num_soldiers = 400;
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

                    debug_spawn_castle(  0.f,   0.f, TEAM_PLAYER, assets);
                    debug_spawn_castle(  0.f,  10.f, TEAM_PLAYER, assets);
                    debug_spawn_castle(-10.f,   5.f, TEAM_PLAYER, assets);
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
                            m4x4 viewproj = camera->VP;
                            f32 w = (f32)game_state->window_width;
                            f32 h = (f32)game_state->window_height;

                            f32 min_screen_x = min(drag_start.x, current_mouse_position.x);
                            f32 min_screen_y = min(drag_start.y, current_mouse_position.y);
                            f32 max_screen_x = max(drag_start.x, current_mouse_position.x);
                            f32 max_screen_y = max(drag_start.y, current_mouse_position.y);

                            Ray3 ray1 = ray_from_screen_position(drag_start, w, h, viewproj);
                            Ray3 ray2 = ray_from_screen_position(current_mouse_position, w, h, viewproj);
                            v3 n = v3{0,1,0};
                            f32 d = 0.f;
                            v3 p1 = {};
                            v3 p2 = {};
                            bool intersects = (ray_plane_intersect(ray1, n, d, &p1) && ray_plane_intersect(ray2, n, d, &p2));
                            if (intersects) {
                                f32 min_x = min(p1.x, p2.x) - game_state->max_radius;
                                f32 min_z = min(p1.z, p2.z) - game_state->max_radius;
                                f32 max_x = max(p1.x, p2.x) + game_state->max_radius;
                                f32 max_z = max(p1.z, p2.z) + game_state->max_radius;
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

                                        v3 ndc = project(entity->position, camera->VP);
                                        f32 x = ( ndc.x * 0.5f + 0.5f) * w;
                                        f32 y = (-ndc.y * 0.5f + 0.5f) * h;
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
                    f32 w = (f32)game_state->window_width;
                    f32 h = (f32)game_state->window_height;

                    v4 color = v4{1.0f, 1.0f, 1.0f, 0.2f};
                    f32 thickness = 1.f;
                    f32 min_x = min(drag_start.x, current_mouse_position.x);
                    f32 min_y = min(drag_start.y, current_mouse_position.y);
                    f32 max_x = max(drag_start.x, current_mouse_position.x);
                    f32 max_y = max(drag_start.y, current_mouse_position.y);
                    render_quad_c(v2{min_x - thickness, min_y - thickness}, v2{max_x + thickness, min_y}, color);
                    render_quad_c(v2{max_x, min_y}, v2{max_x + thickness, max_y}, color);
                    render_quad_c(v2{min_x - thickness, min_y}, v2{min_x, max_y}, color);
                    render_quad_c(v2{min_x - thickness, max_y}, v2{max_x + thickness, max_y + thickness}, color);
                }
            }


            // Draw chunks
            //
            if (draw_chunk_partitions) {
                f32 half_dim_x = 0.5f * game_state->chunk_count_x * game_state->chunk_size.x;
                f32 half_dim_y = 0.5f * game_state->chunk_count_y * game_state->chunk_size.y;

                f32 alpha = 0.7f;

                for (int cy = 0; cy < game_state->chunk_count_y; ++cy) {
                    f32 y = -half_dim_y + game_state->chunk_size.y * cy;
                    draw_line(render_group, v3{-half_dim_x,0.2f,y}, v3{half_dim_x,0.0f,y}, v4{1.f,0.3f,0.3f,alpha});
                }

                for (int cx = 0; cx < game_state->chunk_count_x; ++cx) {
                    f32 x = -half_dim_x + game_state->chunk_size.x * cx;
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
        r_end(g_rhi_state, renderer);

        // Close app if needed
        list_for(os->first_event, event) {
            // Alt + F4?
            bool alt_f4_pressed         = event->kind == OS_EVENT_PRESS && event->key == KEY_F4 && (event->modifiers & OS_MODIFIER_ALT);
            bool window_close_triggered = event->kind == OS_EVENT_WINDOW_CLOSE && event->window == game_state->main_window;

            if (alt_f4_pressed | window_close_triggered) {
                should_close = true;
                os_remove_event(event);
            }
        }

        clear_temporary_storage();
    }

    return 0;
}
