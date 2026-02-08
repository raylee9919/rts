// Copyright Seong Woo Lee. All Rights Reserved.


// @Todo: We are testing our metaprogramming currently. Those defines are kind of 
//        API for serializing data types of entity known to serialization module.
//#define BEGIN_ENTITY
//#define END_ENTITY

// [.h]
//
#include "embed_profiler.h"

#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_random.h"
#include "rts_ds.h"
#include "rts_platform.h"
#include "rts_font.h"
#include "rts_asset.h"
#include "rts_cdt.h"
#include "rts.h"
#include "rts_entity.h"
#include "rts_geogen.h"
#include "renderer/rts_renderer.h"
#include "ui/ui_inc.h"
#include "rect_pack/rpk.h"
#include "font_provider/fp_inc.h"
#include "third_party/stb/stb_image.h"


global Game_State* game_state;
global Renderer* renderer;


// [.cpp]
//
#include "third_party/xxhash3/xxhash.c"
#include "base/rts_base_inc.cpp"
#include "rts_random.cpp"
#include "rts_ds.cpp"
#include "rts_font.cpp"
#include "rts_asset.cpp"
#include "rts_cdt.cpp"
#include "rts_entity.cpp"
#include "renderer/rts_renderer.cpp"
#include "rts_geogen.cpp"
#include "ui/ui_inc.cpp"
#include "rect_pack/rpk.cpp"
#include "font_provider/fp_inc.cpp"
#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

internal Entity*
debug_spawn_soldier(f32 x, f32 z, Team team, Game_Assets* assets)
{
    Entity* soldier            = entity_alloc();
    soldier->type              = ENTITY_TYPE_SOLDIER;
    soldier->flags             = ENTITY_FLAG_IS_UNIT | ENTITY_FLAG_CHUNK_PARTITIONED | ENTITY_FLAG_COLLIDEABLE;
    soldier->team              = team;

    soldier->radius            = 0.5f;
    soldier->speed             = 5.0f;

    soldier->min_t             = 0.0f;
    soldier->max_t             = 0.5f;

    soldier->attack_max_t      = 1.0f;

    soldier->hitpoints         = 40.f;

    soldier->position          = v3(x, 0.f, z);
    soldier->orientation       = Quaternion{1,0,0,0};
    soldier->scaling           = v3(1.f);
    soldier->model             = assets->skeleton_model;
    soldier->idle_animation    = assets->xbot_idle;
    soldier->running_animation = assets->xbot_run;
    soldier->die_animation     = assets->xbot_die;
    soldier->attack_animation  = assets->xbot_attack;
    // @Hack:
    soldier->animation_transform = push_array(game_state->entity_arena, m4x4, soldier->model->node_count);
    entity_init(soldier, nullptr);

    return soldier;
}


extern "C" __declspec(dllexport)
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    ProfileFrameMark;

    if (!os) {
        os = platform->os;
    }
    
    if (!renderer) {
        renderer = platform->renderer;
        render_init();
    }
    
    game_state = (Game_State *)platform->game_state;
    if (!game_state) {
        platform->game_state = game_state = push_struct(platform->arena, Game_State);
    }

    // Update draw/window dimension.
    // @Todo: Switch to immediate-mode?
    game_state->draw_width    = platform->draw_width;
    game_state->draw_height   = platform->draw_height;
    game_state->window_width  = platform->window_width;
    game_state->window_height = platform->window_height;

    // @Todo: We'll deal with timestep later.
    const f32 dt = platform->dt;
    //constexpr f32 dt = 1.f / 60.f;
    
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

        game_state->entity_arena         = arena_alloc();
        game_state->root_entity          = push_struct(game_state->entity_arena, Entity);
        game_state->root_entity->type    = ENTITY_TYPE_ROOT;
        game_state->entity_table_size    = 1024; // FIX: Memory bug in arena when set size to 4096
        game_state->entity_table         = push_array(game_state->entity_arena, Entity, game_state->entity_table_size);
        game_state->next_generational_id = 1;
        
        { // TEMPORARY
            Temporary_Arena scratch = scratch_begin();
            defer(scratch_end(scratch));
            
            Game_Assets *assets = game_state->assets;
            Arena *asset_arena = game_state->assets->arena;
            
            assets->sphere_model = push_struct(asset_arena, Model);
            asset_load_model(assets->sphere_model, utf8f(scratch.arena, "%S/mesh/sphere.smsh", platform->data_path), asset_arena);
            
            assets->plane_model = push_struct(asset_arena, Model);
            {
                asset_load_model(assets->plane_model, utf8f(scratch.arena, "%S/mesh/plane.smsh", platform->data_path), asset_arena);
                asset_load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_albedo.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_normal-ogl.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_roughness.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/wispy-grass-meadow_metallic.sbmp", platform->data_path), asset_arena);
            }
            
            assets->rock_model = push_struct(asset_arena, Model);
            {
                asset_load_model(assets->rock_model, utf8f(scratch.arena, "%S/mesh/rock.smsh", platform->data_path), asset_arena);
                asset_load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/RockAlbedo.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/RockMetalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/RockNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/RockRoughness.sbmp", platform->data_path), asset_arena);
            }

            assets->sword_model = push_struct(asset_arena, Model);
            {
                auto* model = assets->sword_model;
                asset_load_model(model, utf8f(scratch.arena, "%S/mesh/sword.smsh", platform->data_path), asset_arena, v3(0.005f));
                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/sword_albedo.png", platform->data_path), asset_arena);
                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/sword_normal.png", platform->data_path), asset_arena);
                // @Fix
                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/sword_mr.png", platform->data_path), asset_arena);
            }
            
            assets->skeleton_model = push_struct(asset_arena, Model);
            {
                auto* model = assets->skeleton_model;
                asset_load_model(model, utf8f(scratch.arena, "%S/mesh/skeleton_lord.smsh", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[7].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/bodyColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[7].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/bodyMetalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[7].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/bodyNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[7].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/bodyRoughness.sbmp", platform->data_path), asset_arena);
                
                asset_load_image(&model->meshes[4].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/clothColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[4].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/clothNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[4].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/clothRoughness.sbmp", platform->data_path), asset_arena);
                
                asset_load_image(&model->meshes[9].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/helmetColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[9].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/helmetNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[9].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/helmetMetalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[9].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/helmetRoughness.sbmp", platform->data_path), asset_arena);
                
                asset_load_image(&model->meshes[1].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/swordColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[1].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/swordNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[1].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/swordMetalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&model->meshes[1].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/swordRoughness.sbmp", platform->data_path), asset_arena);
                
                assets->xbot_idle = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_idle, utf8f(scratch.arena, "%S/animation/skeleton_lord_idle.sanm", platform->data_path), asset_arena);
                
                assets->xbot_run = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_run, utf8f(scratch.arena, "%S/animation/skeleton_lord_run.sanm", platform->data_path), asset_arena);
                
                assets->xbot_die = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_die, utf8f(scratch.arena, "%S/animation/skeleton_lord_die.sanm", platform->data_path), asset_arena);
                
                assets->xbot_attack = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_attack, utf8f(scratch.arena, "%S/animation/skeleton_lord_attack.sanm", platform->data_path), asset_arena);
            }

            assets->warrior_model = push_struct(asset_arena, Model);
            {
                auto* model = assets->warrior_model;
                asset_load_model(model, utf8f(scratch.arena, "%S/mesh/warrior.smsh", platform->data_path), asset_arena);

                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/warrior_albedo.png", platform->data_path), asset_arena);
                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/warrior_normal.png", platform->data_path), asset_arena);
                // @Todo: Those two have 4 channels each which aren't compatible with our roughness/metalic renderer.
                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/warrior_metalic.png", platform->data_path), asset_arena);
                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/warrior_roughness.png", platform->data_path), asset_arena);
                
                assets->warrior_idle = push_struct(asset_arena, Animation);
                asset_load_animation(assets->warrior_idle, utf8f(scratch.arena, "%S/animation/warrior_idle.sanm", platform->data_path), asset_arena);
            }

            assets->castle_model = push_struct(asset_arena, Model);
            {
                auto* model = assets->castle_model;
                asset_load_model(model, utf8f(scratch.arena, "%S/mesh/castle.smsh", platform->data_path), asset_arena, v3(8.f));

                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/castle_albedo.png", platform->data_path), asset_arena);
                asset_load_image_general_format(&model->meshes[0].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/castle_normal.png", platform->data_path), asset_arena);
            }
            
            assets->crate_model = push_struct(asset_arena, Model);
            {
                asset_load_model(assets->crate_model, utf8f(scratch.arena, "%S/mesh/crate.smsh", platform->data_path), asset_arena);
                asset_load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/crate_albedo.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/crate_normal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/crate_metalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/crate_roughness.sbmp", platform->data_path), asset_arena);
            }
            
            asset_load_image(&game_state->assets->debug_bitmap, utf8f(scratch.arena, "%S/textures/doggo.sbmp", platform->data_path), asset_arena);
            
            char *skybox_filenames[6] = {"right", "left", "top", "bottom", "front", "back"};
            for (u32 i = 0; i < 6; ++i) 
            {
                asset_load_image(assets->skybox_textures + i,
                                 utf8f(scratch.arena, "%S/textures/%s.sbmp", platform->data_path, skybox_filenames[i]),
                                 asset_arena);
            }

            game_state->map_arena     = arena_alloc();
            game_state->chunk_size    = v2u{3,3};
            game_state->chunk_count_x = 128;
            game_state->chunk_count_y = 128;
            game_state->map_size.x    = game_state->chunk_size.x*game_state->chunk_count_x;
            game_state->map_size.y    = game_state->chunk_size.y*game_state->chunk_count_y;
            game_state->chunks        = push_array(game_state->map_arena, Chunk, game_state->chunk_count_x*game_state->chunk_count_y);
            
            {
                Entity *game_camera       = entity_alloc();
                game_camera->type         = ENTITY_TYPE_CAMERA;
                game_camera->focal_length = 0.5f;
                game_camera->N            = 0.5f;
                game_camera->F            = 100000.0f;
                game_camera->position     = v3(0.f, 10.f, 8.f);
                game_camera->orientation  = euler_to_quaternion(radian_from_degree(-55.f), 0.f, 0.f);
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
            constexpr int num_soldiers = 10;
            for (int i = 0; i < num_soldiers*num_soldiers; ++i) {
                f32 x = 6.f /*+ 1.f*(i%num_soldiers)*/;
                f32 z = 0.f + 1.f*(i/num_soldiers);
                Entity* soldier = debug_spawn_soldier(x, z, TEAM_PLAYER, assets);


                // Create a sword and attach it to soldier.
                //
#if 0
                Entity* sword = entity_alloc();
                sword->type              = ENTITY_TYPE_SWORD;
                sword->position          = v3(0.f, 0.f, 0.f);
                sword->orientation       = Quaternion(1.f, 0.f, 0.f, 0.f);
                sword->scaling           = v3(1.f, 1.f, 1.f);
                sword->local_position    = v3(0.f, 0.f, 0.f);
                sword->local_orientation = euler_to_quaternion(radian_from_degree(180.f), 0.f, 0.f);
                sword->model             = game_state->assets->sword_model;
                entity_init(sword, soldier);

                // @Temporary
                constexpr Joint_Id joint_id = 50;
                entity_attach(sword, soldier, joint_id);
#endif
            }

            for (int i = 0; i < num_soldiers*num_soldiers; ++i) {
                f32 x = 8.f;
                f32 z = 0.f + 1.f*(i/num_soldiers);
                Entity* soldier = debug_spawn_soldier(x, z, TEAM_ENEMY, assets);
            }


            { // @Temporary: Create a castle.
                Entity* castle = entity_alloc();
                castle->type   = ENTITY_TYPE_CASTLE;
                castle->flags  = ENTITY_FLAG_CHUNK_PARTITIONED;

                castle->position    = V3(0.f,0.f,0.f);
                castle->orientation = Quaternion{1,0,0,0};
                castle->scaling     = V3(1.f);
                castle->model       = assets->castle_model;

                castle->navmesh_scale = 3.f;

                entity_init(castle, nullptr);
            }
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

    // NOTE: Alias
    Game_Assets *assets = game_state->assets;
    
    Render_Group* render_group = begin_render_group(render_commands, MB(16));

    
    // Get all triangles in the navmesh
    //
    int triangle_count = cdt_get_triangle_count(&game_state->navmesh.ctx);
    game_state->navmesh.triangles = push_array(game_state->frame_arena, cdt_triangle, triangle_count);
    cdt_get_all_triangles(&game_state->navmesh.ctx, game_state->navmesh.triangles);
    game_state->navmesh.triangle_count = triangle_count;

    
    
    // TEMPORARY: Testing UI
    //            1. Interact with UI built in last frame.
    //            2. Build new hierarchy while retaining some data(!!!)
    //
    local_persist f32 light_x = 1.f;
    local_persist f32 light_y = 1.f;
    local_persist f32 light_z = 1.f;
    ui_begin(dt, platform->window_width, platform->window_height);
    {
        ui_platform(utf8lit("⚙"))
        {
            ui_labelf("mspf: %.2f", dt*1000.f);
            ui_slider_f32(&ui_state->font_size, 8.f, 30.f, utf8lit("Font Size"));
            if (ui_button(utf8lit("Wireframe")).pressed_left) {
                render_commands->wireframe_mode = !render_commands->wireframe_mode; 
            }
            if (ui_button(utf8lit("Navmesh")).pressed_left) {
                render_commands->draw_navmesh = !render_commands->draw_navmesh; 
            }
            
            if (ui_expander(utf8lit("Shadow"))) {
                ui_slider_f32(&light_x, -1.0f, 1.0f, utf8lit("x"));
                ui_slider_f32(&light_y,  0.1f, 1.0f, utf8lit("y"));
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


    // Update entities
    //
    entity_update(game_state->root_entity, dt);


    // Draw entities
    //
    for (u32 i = 0; i < game_state->entity_table_size; ++i) {
        Entity* bucket = game_state->entity_table + i;
        for (Entity* entity = bucket->first; entity; entity = entity->next_in_table) {
            entity_draw(entity, dt, render_group, render_commands);
        }
    }

    // Draw ground
    //
    Mesh *ground_mesh = assets->plane_model->meshes;
    f32 gx = game_state->map_size.x;
    f32 gy = game_state->map_size.y;
    m4x4 ground_transform = {{
        {gx,0, 0, 0},
        {0, 1, 0, 0},
        {0, 0,gy, 0},
        {0, 0, 0, 1},
    }};
    push_mesh(renderer, ground_mesh, ground_transform, 0, 0, v2{gx,gy});

    
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
            draw_line(render_group, V3(a.y, 0.f, a.x), V3(b.y, 0.f, b.x), color);
        }
    }

    { // NOTE: Render Commands
        Entity* game_camera = entity_from_id(game_state->game_camera_id);
        Entity* controlling_camera = entity_from_id(game_state->controlling_camera_id);

        render_commands->main_eye_position = controlling_camera->position;
        render_commands->main_view_proj    = controlling_camera->VP;
        
        render_commands->wireframe_color = V4(0.9f, 0.9f, 0.9f, 1.0f);
        
        // NOTE: Skybox
        //
        render_commands->skybox_on = true;
        render_commands->skybox_mesh = &assets->skybox_mesh;
        render_commands->skybox_eye_view_proj = controlling_camera->VP;
        for (u32 i = 0; i < 6; ++i) 
        {
            render_commands->skybox_textures[i] = assets->skybox_textures + i;
        }
        
        
        // NOTE: CSM
        //
        render_commands->csm_to_light = normalize(v3{light_x, light_y, light_z});
        f32 csm_frustum_edge_length = 50.0f;
        m4x4 inv = inverse(game_camera->VP);
        // TODO: Renderer independent calculation!
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
