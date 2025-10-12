/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// @Todo: We are testing our metaprogramming currently. Those defines are kind of 
//         API for serializing data types of entity known to serialization module.
#define BEGIN_ENTITY
#define END_ENTITY


// @Note: [.h]
//
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_random.h"
#include "rts_platform.h"
#include "rts_font.h"
#include "rts_asset.h"
#include "rts_ds.h"
#include "rts_delaunay.h"
#include "rts_nav.h"
#include "rts.h"
#include "rts_geogen.h"
#include "renderer/rts_renderer.h"
#include "ui/ui_inc.h"
#include "generated/entity.h"
#include "generated/entity_serialization.h"
#include "rts_map_loader.h"
#include "rts_sim.h"
#include "rect_pack/rpk.h"
#include "font_provider/fp_inc.h"

// @Note: Globals.
//
global Renderer *renderer;

// @Note: [.cpp]
//
#include "base/rts_base_inc.cpp"
#include "rts_random.cpp"
#include "rts_font.cpp"
#include "rts_asset.cpp"
#include "renderer/rts_renderer.cpp"
#include "rts_geogen.cpp"
#include "ui/ui_inc.cpp"
#include "rts_delaunay.cpp"
#include "rts_nav.cpp"
#include "rts_sim.cpp"
#include "rts_map_loader.cpp"
#include "rect_pack/rpk.cpp"
#include "font_provider/fp_inc.cpp"


internal void
update_entities(World *world, Game_State *game_state)
{
    for (u32 idx = 0; idx < world->entity_count; ++idx) 
    {
        Entity *entity = world->entities[idx];
        if (entity->id != 0) 
        {
            entity->update(entity, game_state);
        }
    }
}

internal void
draw_entities(Game_State *game_state, Render_Commands *commands, Render_Group *render_group, Render_Group *orthographic_group, v3 center, v3 draw_dim)
{
    World *world = game_state->world;
    for (u32 idx = 0; idx < world->entity_count; ++idx) 
    {
        Entity *entity = world->entities[idx];
        if (entity->id != 0) 
        {
            entity->draw(entity, game_state, commands, render_group, orthographic_group);
        }
    }
}

internal void
update_cameras(World *world, Game_State *game_state)
{
    for (u32 idx = 0; idx < world->camera_count; ++idx) 
    {
        Camera *camera = world->cameras[idx];
        camera->update((Entity *)camera, game_state);
    }
}

no_name_mangle
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    // @Note: Acquire os.
    //
    if (os == NULL)
    {
        os = platform->os;
    }

    // @Note: Acquire renderer.
    //
    if (renderer == NULL)
    {
        renderer = platform->renderer;
        render_init();
    }

    // @Note: Acquire game state.
    //
    Game_State *game_state = (Game_State *)platform->game_state;
    if (! game_state) 
    {
        platform->game_state = game_state = push_struct(platform->arena, Game_State);
    }

    {
        game_state->draw_width    = platform->draw_width;
        game_state->draw_height   = platform->draw_height;
        game_state->window_height = platform->window_width;
        game_state->window_height = platform->window_height;
        // @Todo: Warn on out-out-range refresh.
        game_state->dt_real = clamp(platform->dt, 0.001f, 0.1f);
        game_state->dt_game = game_state->dt_real;
        game_state->time   += game_state->dt_real;
    }

    if (! game_state->initted) 
    {
        game_state->initted = true;

        thread_init();

        // @Note: alloc assets
        //
        Arena *arena = arena_alloc();
        game_state->assets = push_struct(arena, Game_Assets);
        game_state->assets->arena = arena;


        game_state->frame_arena = arena_alloc();

        // @Note: init world.
        //
        Arena *world_arena = arena_alloc();
        World *world = game_state->world = push_struct(world_arena, World);
        world->arena = world_arena;
        world->next_entity_id = 1;

        game_state->mode = GAME_MODE_GAME;
        game_state->random_series = rand_seed(1219);

        { // @Temporary
            Temporary_Arena scratch = scratch_begin();
            scope_exit(scratch_end(scratch));

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

            assets->xbot_model = push_struct(asset_arena, Model);
            {
                asset_load_model(assets->xbot_model, utf8f(scratch.arena, "%S/mesh/skeleton_lord.smsh", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/bodyColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/bodyMetalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/bodyNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/bodyRoughness.sbmp", platform->data_path), asset_arena);

                asset_load_image(&assets->xbot_model->meshes[4].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/clothColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[4].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/clothNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[4].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/clothRoughness.sbmp", platform->data_path), asset_arena);

                asset_load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/helmetColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/helmetNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/helmetMetalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/helmetRoughness.sbmp", platform->data_path), asset_arena);

                asset_load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Albedo], utf8f(scratch.arena, "%S/textures/swordColor.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Normal], utf8f(scratch.arena, "%S/textures/swordNormal.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Metalic], utf8f(scratch.arena, "%S/textures/swordMetalic.sbmp", platform->data_path), asset_arena);
                asset_load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Roughness], utf8f(scratch.arena, "%S/textures/swordRoughness.sbmp", platform->data_path), asset_arena);

                assets->xbot_idle = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_idle, utf8f(scratch.arena, "%S/animation/skeleton_lord_idle.sanm", platform->data_path), asset_arena);

                assets->xbot_run = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_run, utf8f(scratch.arena, "%S/animation/skeleton_lord_run.sanm", platform->data_path), asset_arena);

                assets->xbot_die = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_die, utf8f(scratch.arena, "%S/animation/skeleton_lord_die.sanm", platform->data_path), asset_arena);

                assets->xbot_attack = push_struct(asset_arena, Animation);
                asset_load_animation(assets->xbot_attack, utf8f(scratch.arena, "%S/animation/skeleton_lord_attack.sanm", platform->data_path), asset_arena);
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

            // @Temporary: init cameras.
            {
                game_state->game_camera = push_entity(world, Camera, V3(0,0,0));
                {
                    game_state->game_camera->init(Camera_Type_Perspective, 0.5f, 0.5f, 100000.0f, world);
                    game_state->game_camera->orientation = euler_to_quaternion(degrees_to_radian(-45), 0, 0);
                    game_state->game_camera->position = v3{0,5,5};
                }

                game_state->debug_camera = push_entity(world, Camera, V3(0,0,0));
                {
                    game_state->debug_camera->init(Camera_Type_Perspective, 0.5f, 0.5f, 100000.0f, world);
                    game_state->debug_camera->orientation = euler_to_quaternion(degrees_to_radian(-45), 0, 0);
                    game_state->debug_camera->position += game_state->game_camera->position + v3{0,5,5};
                }

                game_state->orthographic_camera = push_entity(world, Camera, V3(0,0,0));
                {
                    game_state->orthographic_camera->init(Camera_Type_Orthographic, 0, -100, 100, world);
                }

                //game_state->controlling_camera = game_state->debug_camera;
                game_state->controlling_camera = game_state->game_camera;
            }


            geogen_backfaced_cube(&assets->skybox_mesh, asset_arena, 10000);

            load_map(utf8f(scratch.arena, "%S/map/map1.smap", platform->data_path), game_state);

            render_commands->csm_varient_method = true;

            // @Temporary: We are setting navmesh input data by hand.
            {
                Arena *arena = arena_alloc();
                game_state->navmesh = push_struct(arena, Navmesh);
                game_state->navmesh->arena = arena;
            }
            Navmesh *navmesh = game_state->navmesh;
            navmesh->vertex_size = 1000;
            navmesh->vertices = push_array(navmesh->arena, Vertex, navmesh->vertex_size);

            navmesh->constrain_size = 1000;
            navmesh->constrains = push_array(navmesh->arena, Nav_Constrain, navmesh->constrain_size);

            for (u32 i = 0; i < navmesh->constrain_size; ++i) 
            {
                navmesh->constrains[i].edge_count = 0;
                navmesh->constrains[i].edge_size = 1000;
                navmesh->constrains[i].edges = (int(*)[2])push_size(navmesh->arena, sizeof(int)*2*navmesh->constrains[i].edge_size);
            }

            push_vertex(navmesh, v3{-50,0,-50});
            push_vertex(navmesh, v3{-50,0, 50});
            push_vertex(navmesh, v3{ 50,0, 50});
            push_vertex(navmesh, v3{ 50,0,-50});

            for (u32 i = 0; i < game_state->world->entity_count; ++i) 
            {
                Entity *e = game_state->world->entities[i];
                if (e->flags & Flag_Navmesh) 
                {
                    m4x4 transform = translate(scale(identity(), e->scaling), e->position);
                    begin_constrain(navmesh);
                    push_vertex(navmesh, (transform*v4{-1.f, 0.f, -1.f, 1.f}).xyz);
                    push_vertex(navmesh, (transform*v4{-1.f, 0.f,  1.f, 1.f}).xyz);
                    push_vertex(navmesh, (transform*v4{ 1.f, 0.f,  1.f, 1.f}).xyz);
                    push_vertex(navmesh, (transform*v4{ 1.f, 0.f, -1.f, 1.f}).xyz);
                    end_constrain(navmesh);
                }
            }

            // @Todo: not neat.
            begin_constrain(navmesh);
            {
                push_vertex(navmesh, v3{ 1.f, 0.f,  3.f});
                push_vertex(navmesh, v3{ 3.f, 0.f,  2.f});
                push_vertex(navmesh, v3{ 2.f, 0.f, -2.f});
                push_vertex(navmesh, v3{-1.f, 0.f, -4.f});
                push_vertex(navmesh, v3{-3.f, 0.f, -3.f});
                push_vertex(navmesh, v3{-4.f, 0.f,  1.f});
            }
            end_constrain(navmesh);

            for (u32 i = 0; i < navmesh->vertex_count; ++i)
            {
                navmesh->vertices[i].position.y = 0.01f;
            }

            navmesh->cdt = delaunay_triangulate(navmesh->vertices, navmesh->vertex_count, navmesh);
        }
    }

    { // @Note: Begin
        arena_clear(game_state->frame_arena);
        render_begin();
    }

    if (fp_state == NULL)
    {
        fp_state = fp_alloc();
        fp_init();
    }


    // @Note: ui alloc/init
    //
    if (ui_state == NULL)
    {
        ui_state = ui_alloc();
        ui_init(ui_state);
    }

    // @Temporary:
    game_state->active_entity_id = render_commands->active_entity_id;
    game_state->view_proj = game_state->controlling_camera->VP;

    // @Note: Alias
    World *world = game_state->world;
    Game_Assets *assets = game_state->assets;

    Render_Group *render_group       = begin_render_group(render_commands, MB(16));
    Render_Group *orthographic_group = begin_render_group(render_commands, MB(16));

    // @Temporary:
    Navmesh *navmesh = game_state->navmesh;
    Cdt_Result *cdt = &navmesh->cdt;

    if (render_commands->draw_navmesh) 
    {
        draw_triangles(render_group, navmesh->vertices, navmesh->vertex_count, (u32 *)cdt->tri, cdt->numtri, V4(V3(0.3f),1.0f));

        for (int i = 0; i < navmesh->cdt.numtri; ++i) 
        {
            char buf[256];
            str_snprintf(buf, sizeof(buf), "%d", i);

            v4 tmp = game_state->controlling_camera->VP * V4(get_centroid(navmesh, i), 1);
            v3 projected_position = (tmp.xyz / tmp.w);
            projected_position.xy = hadamard(binormal_to_normal(projected_position.xy), v2{(f32)platform->draw_width, (f32)platform->draw_height});
        }
    }


    // @Temporary: Testing UI
    //              1. Interact with UI built in last frame.
    //              2. Build new hierarchy while retaining some data(!!!)
    //
    local_persist f32 light_x = 1.f;
    local_persist f32 light_y = 1.f;
    local_persist f32 light_z = 1.f;
    ui_begin(platform->dt, platform->window_width, platform->window_height);
    {
        ui_platform(utf8lit("⚙Developer"))
        {

            if (ui_button(utf8lit("Wireframe")))
            {
                render_commands->wireframe_mode = !render_commands->wireframe_mode; 
            }
            if (ui_button(utf8lit("Navmesh")))
            {
                render_commands->draw_navmesh = !render_commands->draw_navmesh; 
            }
            if (ui_drop(utf8lit("드랍다운(Dropdown)")))
            {
                ui_slider_f32(&light_x, 0.0001f, 1.f, utf8lit("Light X"));
                ui_slider_f32(&light_y, 0.0001f, 1.f, utf8lit("Light Y"));
                ui_slider_f32(&light_z, 0.0001f, 1.f, utf8lit("Light Z"));
                if (ui_button(utf8lit("Valient's Method")))
                {
                    render_commands->csm_varient_method = !render_commands->csm_varient_method; 
                }
                if (ui_button(utf8lit("Csm Frustum")))
                {
                    render_commands->draw_csm_frustum = !render_commands->draw_csm_frustum; 
                }
            }
            if (ui_button(utf8lit("カメラ(Camera)"))) 
            {
                if (game_state->controlling_camera == game_state->game_camera) 
                {
                    game_state->controlling_camera = game_state->debug_camera;
                } 
                else 
                {
                    game_state->controlling_camera = game_state->game_camera;
                }
            }
        }
    }
    ui_end();



    switch (game_state->mode) 
    {
        case GAME_MODE_GAME: 
        {
            update_entities(world, game_state);
            draw_entities(game_state, render_commands, render_group, orthographic_group, game_state->controlling_camera->position, V3(100));
        } break;

        case GAME_MODE_EDITOR: 
        {
            if (! game_state->editor_initted) 
            {
                game_state->editor_initted = true;
                update_entities(world, game_state);
                game_state->controlling_camera = game_state->debug_camera;
            }

#if 0
            ui_dev(render_commands, game_state, input);
            ui.begin("Editor Panel", V2(0.6f, 0.7f));
            v4 color = V4(0.2f,0.8f,0.2f,0.4f);
            if (ui.button(color, "Play")) 
            {
                game_state->mode = Game_Mode_Game;
                game_state->controlling_camera = game_state->game_camera;
            }

            if (ui.button(color, "Save")) 
            {
                Temporary_Arena scratch = scratch_begin();
                scope_exit(scratch_end(scratch));

                Date_Time time = os.date_time_current();
                Utf8 map_save_path = utf8f(scratch.arena, "%S/map/map1.smap", platform->data_path);
                Utf8 map_back_path = utf8f(scratch.arena, "%S/map/backup/map1_%d_%d_%d_%d_%d_%d.smap", platform->data_path, time.year, time.month, time.day, time.hour, time.minute, time.second);
                os.file_copy(map_back_path, map_save_path);

                FILE *file = fopen((char *)map_save_path.str, "wb");
                Assert(file);
                for (u32 entityidx = 0; entityidx < world->entity_count; ++entityidx) 
                {
                    Entity *entity = world->entities[entityidx];
                    if (entity->serialize) 
                    {
                        entity->serialize(entity, game_state, file);
                    }
                }
                fclose(file);
                ui.fadeout_text(V4(1.0f), "Save");
            }
            ui.end();
#endif

            // no update entities in editor mode.
            update_cameras(world, game_state);
            draw_entities(game_state, render_commands, render_group, orthographic_group, game_state->controlling_camera->position, V3(100));
        } break;

        default: {
            assert(! "invalid default case"); 
        } break;
    }

    { // @Note: Render Commands
        render_commands->main_eye_position = game_state->controlling_camera->position;
        render_commands->main_view_proj = game_state->controlling_camera->VP;
        render_commands->ortho_view_proj = game_state->orthographic_camera->VP;

        render_commands->wireframe_color = V4(0.9f, 0.9f, 0.9f, 1.0f);

        // @Note: Skybox
        //
        render_commands->skybox_on = true;
        render_commands->skybox_mesh = &assets->skybox_mesh;
        render_commands->skybox_eye_view_proj = game_state->controlling_camera->VP;
        for (u32 i = 0; i < 6; ++i) 
        {
            render_commands->skybox_textures[i] = assets->skybox_textures + i;
        }


        // @Note: CSM
        //
        render_commands->csm_to_light = normalize(v3{light_x, light_y, light_z});
        f32 csm_frustum_edge_length = 50.0f;
        m4x4 inv = inverse(game_state->game_camera->VP);
        // @Todo: Renderer independent calculation!
        v4 ndcs[4] = {
            v4{-1,-1,-1, 1},
            v4{ 1,-1,-1, 1},
            v4{-1, 1,-1, 1},
            v4{ 1, 1,-1, 1},
        };

        v3 eye = game_state->game_camera->position;
        v4 positions[8];

        for (u32 i = 0; i < 4; ++i) 
        {
            positions[i] = inv * ndcs[i];
            positions[i].xyz *= (1.f / positions[i].w);
        }

        for (u32 i = 0; i < 4; ++i) 
        {
            v3 d = normalize(positions[i].xyz - eye);
            positions[4+i] = positions[i];
            positions[4+i].xyz += (csm_frustum_edge_length*d);
        }

        for (u32 i = 0; i < 8; ++i) 
        {
            render_commands->csm_frustum_positions[i] = positions[i].xyz;
        }
        render_commands->csm_view = game_state->game_camera->V;
    }




    render_end();
}
