/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// @Todo: Remove
#define BEGIN_ENTITY
#define END_ENTITY

// @Note: [.h]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_math.h"
#include "rts_random.h"
#include "rts_color.h"
#include "rts_platform.h"
#include "rts_asset.h"

#include "input.h"

#include "queue.h"
#include "priority_queue.h"

#include "rts_ui.h"
Ui ui;

#include "rts_delaunay.h"
#include "nav.h"
#include "rts_game.h"

#include "renderer.h"
#include "renderer.cpp"
#include "generated/entity.h"
#include "generated/entity_serialization.h"
#include "rts_sim.h"
#include "input.cpp"
#include "console.cpp"

#include "debug.h"

// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "rts_math.cpp"
#include "rts_random.cpp"
#include "rts_asset.cpp"
#include "rts_ui.cpp"
#include "rts_delaunay.cpp"
#include "rts_sim.cpp"

#define GROUND_RES 20

#define WORLD_RESOLUTION 16
#define GROUND_SIZE 100

#define ASSET_FONT(NAME) "font/"#NAME".sfnt"

internal void
update_game_mode(Game_State *game_state, Input *input) 
{
    switch (game_state->mode) 
    {
        case Game_Mode_Game: {
            if (toggled_down(input, KEY_ESC)) {
                game_state->mode = Game_Mode_Menu;
            }
        } break;

        case Game_Mode_Menu: {
            if (toggled_down(input, KEY_ESC)) {
                game_state->mode = Game_Mode_Game;
            }
        } break;

        case Game_Mode_Editor: { 
        } break;

        INVALID_DEFAULT_CASE;
    }
}

internal void
update_entities(World *world, Game_State *game_state, Input *input)
{
    for (u32 idx = 0; idx < world->entity_count; ++idx) 
    {
        Entity *entity = world->entities[idx];
        if (entity->id != 0) 
        {
            entity->update(entity, game_state, input);
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
update_and_draw_entity_panel(Game_State *game_state)
{
    for (u32 idx = 0; idx < game_state->world->entity_count; ++idx) 
    {
        Entity *entity = game_state->world->entities[idx];
        if (entity->id != 0) 
        {
            if (entity->panel) 
            {
                entity->panel(entity, game_state);
            }
        }
    }
}

internal void
debug_update_game_speed(Debug_State *debug_state, Input *input)
{
    debug_state->speed_fade_t -= input->actual_dt;
    if (input->keys[KEY_LEFTCTRL].is_down) {
        Debug_State *ds = debug_state;
        if (toggled_down(input, KEY_EQUAL) &&
            ds->current_speed_idx < array_count(ds->speed_slider) - 1) 
        {
            ++ds->current_speed_idx;
            ds->speed = ds->speed_slider[ds->current_speed_idx];
            ds->speed_fade_t = 1.0f;
        }
        if (toggled_down(input, KEY_MINUS) &&
            ds->current_speed_idx != 0) 
        {
            --ds->current_speed_idx;
            ds->speed = ds->speed_slider[ds->current_speed_idx];
            ds->speed_fade_t = 1.0f;
        }
    }
}

internal void
draw_game_speed_text(Debug_State *debug_state, Input *input, Render_Group *render_group, Asset_Font *font)
{
    if (debug_state->speed_fade_t > epsilon_f32) 
    {
        char *text = debug_state->speed_text[debug_state->current_speed_idx];
        v2 dim = get_dim(string_rect(text, {}, font));
        f32 alpha = lerp(0.0f, debug_state->speed_fade_t, 1.0f);
        f32 dy = 30*lerp(1.0f, debug_state->speed_fade_t, 0.0f);
        v2 cen = v2{0.5f * input->draw_dim.w, 0.7f * input->draw_dim.h - dy};
        cen += v2{10, -10};
        string_op(String_Op_Draw, render_group, V3(cen-0.5f*dim, -1), text, font, V4(V3(0.0f), alpha));
        cen -= v2{10, -10};
        string_op(String_Op_Draw, render_group, V3(cen-0.5f*dim, 0), text, font, V4(V3(1.0f), alpha));
    }
}

internal void
debug_draw_performance(Render_Group *render_group, Input *input, Asset_Font *font) 
{
    char buf[256];
    snprintf(buf, 256, "actual mspf: %.4f | fps: %d", 1000.0f*input->actual_dt, (s32)(1.0f/input->actual_dt + 0.5f));
    v3 base = v3{10, input->draw_dim.y - 30.0f, 0};
    string_op(String_Op_Draw, render_group, base + V3(2,-2,-1), buf, font, RGBA_BLACK);
    string_op(String_Op_Draw, render_group, base, buf, font);
}

internal void
update_cameras(World *world, Game_State *game_state, Input *input)
{
    for (u32 idx = 0; idx < world->camera_count; ++idx) 
    {
        Camera *camera = world->cameras[idx];
        camera->update((Entity *)camera, game_state, input);
    }
}

internal void
ui_dev(Render_Commands *render_commands, Game_State *game_state, Input *input)
{
    v4 color = V4(0.2f,0.2f,0.4f,0.6f);
    ui.begin("Dev", V2(-0.98f, 0.7f));
    ui.checkbox(&render_commands->wireframe_mode, color, "Wireframe", "Draw meshes' wireframe.");
    ui.checkbox(&render_commands->draw_navmesh, color, "Navmesh", "Draw Navigation Mesh.");
    ui.checkbox(&render_commands->draw_csm_frustum, color, "CSM Frustum", "Draw frustum volume for cascaded shadow mapping.");
    if (ui.checkbox(&render_commands->csm_varient_method, color, "CSM Valient's Method", "Use Valient's algorithm introduced in Shaderx book.")) 
    { ui.checkbox(&render_commands->draw_csm_sphere, color, "CSM Draw Sphere", "Not implemented."); }
    if (ui.button(color, "Switch Camera", "Switch between game camera and debug camera.")) {
        if (game_state->controlling_camera == game_state->game_camera) {
            game_state->controlling_camera = game_state->debug_camera;
        } else {
            game_state->controlling_camera = game_state->game_camera;
        }
    }
    ui.end();
}

#include "map_loader.cpp"

no_name_mangle
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    Game_State *game_state = (Game_State *)game_memory->game_state;
    if (game_state == 0)
    { game_memory->game_state = game_state = push_struct(game_memory->arena, Game_State); }

    os = game_memory->os;

    f32 draw_width  = input->draw_dim.w;
    f32 draw_height = input->draw_dim.h;

    input->dt = clamp(input->dt, 0.001f, 0.1f); // @Todo: Warn on out-out-range refresh.

    if (! game_state->initted) 
    {
        game_state->initted = true;

        thread_init();

        game_state->asset_arena = arena_alloc();
        game_state->game_assets = push_struct(game_state->asset_arena, Game_Assets);

        game_state->debug_arena = arena_alloc();
        game_state->debug_state = push_struct(game_state->debug_arena, Debug_State);

        game_state->ui_arena = arena_alloc();

        game_state->frame_arena = arena_alloc();

        // -------------------------------------------
        // @Note: init world.
        Arena *world_arena = arena_alloc();
        World *world = game_state->world = push_struct(world_arena, World);
        world->arena = world_arena;
        world->next_entity_id = 1;

        game_state->mode = Game_Mode_Editor;
        game_state->random_series = rand_seed(1219);

        { // @Temporary
            Game_Assets *assets = game_state->game_assets;
            Arena *asset_arena = game_state->asset_arena;

            assets->sphere_model = push_struct(asset_arena, Model);
            load_model(assets->sphere_model, "sphere", asset_arena);

            assets->plane_model = push_struct(asset_arena, Model);
            load_model(assets->plane_model, "plane", asset_arena);

            assets->rock_model = push_struct(asset_arena, Model);
            load_model(assets->rock_model, "rock", asset_arena);
            load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Albedo], "textures/RockAlbedo.sbmp", asset_arena);
            load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Metalic], "textures/RockMetalic.sbmp", asset_arena);
            load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Normal], "textures/RockNormal.sbmp", asset_arena);
            load_image(&assets->rock_model->meshes[0].textures[Pbr_Texture_Roughness], "textures/RockRoughness.sbmp", asset_arena);

            assets->xbot_model = push_struct(asset_arena, Model);
            load_model(assets->xbot_model, "skeleton_lord", asset_arena);
            load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Albedo], "textures/bodyColor.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Metalic], "textures/bodyMetalic.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Normal], "textures/bodyNormal.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[7].textures[Pbr_Texture_Roughness], "textures/bodyRoughness.sbmp", asset_arena);

            load_image(&assets->xbot_model->meshes[4].textures[Pbr_Texture_Albedo], "textures/clothColor.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[4].textures[Pbr_Texture_Normal], "textures/clothNormal.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[4].textures[Pbr_Texture_Roughness], "textures/clothRoughness.sbmp", asset_arena);

            load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Albedo], "textures/helmetColor.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Normal], "textures/helmetNormal.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Metalic], "textures/helmetMetalic.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[9].textures[Pbr_Texture_Roughness], "textures/helmetRoughness.sbmp", asset_arena);

            load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Albedo], "textures/swordColor.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Normal], "textures/swordNormal.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Metalic], "textures/swordMetalic.sbmp", asset_arena);
            load_image(&assets->xbot_model->meshes[1].textures[Pbr_Texture_Roughness], "textures/swordRoughness.sbmp", asset_arena);

            assets->crate_model = push_struct(asset_arena, Model);
            load_model(assets->crate_model, "crate", asset_arena);
            load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Albedo], "textures/crate_albedo.sbmp", asset_arena);
            load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Normal], "textures/crate_normal.sbmp", asset_arena);
            load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Metalic], "textures/crate_metalic.sbmp", asset_arena);
            load_image(&assets->crate_model->meshes[0].textures[Pbr_Texture_Roughness], "textures/crate_roughness.sbmp", asset_arena);

            assets->xbot_idle = push_struct(asset_arena, Animation);
            load_animation(assets->xbot_idle, "skeleton_lord_idle", asset_arena);

            assets->xbot_run = push_struct(asset_arena, Animation);
            load_animation(assets->xbot_run, "skeleton_lord_run", asset_arena);

            assets->xbot_die = push_struct(asset_arena, Animation);
            load_animation(assets->xbot_die, "skeleton_lord_die", asset_arena);

            assets->xbot_attack = push_struct(asset_arena, Animation);
            load_animation(assets->xbot_attack, "skeleton_lord_attack", asset_arena);

            load_font(asset_arena, ASSET_FONT(noto_serif), &assets->debug_font);
            load_font(asset_arena, ASSET_FONT(noto_serif), &assets->console_font);
            load_font(asset_arena, ASSET_FONT(gill_sans), &assets->menu_font);
            load_font(asset_arena, ASSET_FONT(Karmina Regular), &assets->karmina);

            load_image(&game_state->game_assets->debug_bitmap, "../data/textures/doggo.sbmp", asset_arena);

            char *skybox_filenames[6] = {"textures/right.sbmp", "textures/left.sbmp", "textures/top.sbmp", "textures/bottom.sbmp", "textures/front.sbmp", "textures/back.sbmp"};
            for (u32 i = 0; i < 6; ++i) 
            { load_image(assets->skybox_textures + i, skybox_filenames[i], asset_arena); }

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

                game_state->controlling_camera = game_state->debug_camera;
            }

            load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Albedo], "textures/wispy-grass-meadow_albedo.sbmp", asset_arena);
            load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Normal], "textures/wispy-grass-meadow_normal-ogl.sbmp", asset_arena);
            load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Roughness], "textures/wispy-grass-meadow_roughness.sbmp", asset_arena);
            load_image(&assets->plane_model->meshes[0].textures[Pbr_Texture_Metalic], "textures/wispy-grass-meadow_metallic.sbmp", asset_arena);
            generate_backfaced_cube(&assets->skybox_mesh, asset_arena, 10000);

            load_map("map1", game_state);

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
            //navmesh->vertices = (Vertex *)malloc(sizeof(Vertex)*navmesh->vertex_size);

            navmesh->constrain_size = 1000;
            navmesh->constrains = (Nav_Constrain *)malloc(sizeof(Nav_Constrain)*navmesh->constrain_size);
            zero_array(navmesh->constrains, navmesh->constrain_size);
            for (umm i = 0; i < navmesh->constrain_size; ++i) 
            {
                navmesh->constrains[i].edge_count = 0;
                navmesh->constrains[i].edge_size = 1000;
                navmesh->constrains[i].edges = (int (*)[2])malloc(sizeof(int)*2*navmesh->constrains[i].edge_size);
                zero_memory(navmesh->constrains[i].edges, sizeof(int)*2*navmesh->constrains[i].edge_size);
            }

            push_vertex(navmesh, v3{-50,0,-50});
            push_vertex(navmesh, v3{-50,0, 50});
            push_vertex(navmesh, v3{ 50,0, 50});
            push_vertex(navmesh, v3{ 50,0,-50});

            for (umm i = 0; i < game_state->world->entity_count; ++i) {
                Entity *e = game_state->world->entities[i];
                if (e->flags & Flag_Navmesh) {
                    m4x4 transform = translate(scale(identity(), e->scaling), e->position);
                    begin_constrain(navmesh);
                    push_vertex(navmesh, (transform*v4{-1.0f, 0,-1.0f, 1}).xyz);
                    push_vertex(navmesh, (transform*v4{-1.0f, 0, 1.0f, 1}).xyz);
                    push_vertex(navmesh, (transform*v4{ 1.0f, 0, 1.0f, 1}).xyz);
                    push_vertex(navmesh, (transform*v4{ 1.0f, 0,-1.0f, 1}).xyz);
                    end_constrain(navmesh);
                }
            }

            begin_constrain(navmesh);
            push_vertex(navmesh, v3{ 1.0f, 0, 3.0f});
            push_vertex(navmesh, v3{ 3.0f, 0, 2.0f});
            push_vertex(navmesh, v3{ 2.0f, 0,-2.0f});
            push_vertex(navmesh, v3{-1.0f, 0,-4.0f});
            push_vertex(navmesh, v3{-3.0f, 0,-3.0f});
            push_vertex(navmesh, v3{-4.0f, 0, 1.0f});
            end_constrain(navmesh);

            for (umm i = 0; i < navmesh->vertex_count; ++i){
                navmesh->vertices[i].position.y = 0.01f;
            }

            navmesh->cdt = delaunay_triangulate(navmesh->vertices, navmesh->vertex_count, navmesh);
        }
    }
    arena_clear(game_state->frame_arena);

    u64 writetime = os.get_last_write_time(ASSET_FONT(Times New Roman));
    if (writetime != game_state->game_assets->times.writetime) {
        game_state->game_assets->times.writetime = writetime;
        load_font(game_state->asset_arena, ASSET_FONT(Times New Roman), &game_state->game_assets->times);
    }

    game_state->active_entity_id = render_commands->active_entity_id;
    game_state->view_proj = game_state->controlling_camera->VP;

    World *world = game_state->world;
    Game_Assets *assets = game_state->game_assets;
    Debug_State *debug_state = (Debug_State *)game_state->debug_state;
    Console *console = &debug_state->console;

    if (!debug_state->initted) 
    {
        debug_state->initted = true;

        Debug_State *ds = debug_state;
        init_console(&ds->console, &assets->console_font);

        ds->speed = 1.0f;
        ds->current_speed_idx = 3;
        ds->speed_slider[0] = 0.33f;
        ds->speed_slider[1] = 0.50f;
        ds->speed_slider[2] = 0.75f;
        ds->speed_slider[3] = 1.00f;
        ds->speed_slider[4] = 1.50f;

        ds->speed_text[0] = "x0.33";
        ds->speed_text[1] = "x0.5";
        ds->speed_text[2] = "x0.75";
        ds->speed_text[3] = "x1";
        ds->speed_text[4] = "x1.5";
    }

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
            projected_position.xy = hadamard(binormal_to_normal(projected_position.xy), v2{draw_width, draw_height});

            //push_shadowed_string(orthographic_group, projected_position, buf, &game_state->game_assets->times);
        }
    }

    if (! ui.initted) 
    { ui.init(input, game_state->ui_arena, orthographic_group, &assets->times, &assets->menu_font); }

    // @TODO: cleanup
    input->dt *= debug_state->speed;
    game_state->real_time += input->actual_dt;
    update_input_state(input, event_queue, input->actual_dt);

    update_game_mode(game_state, input);
    switch (game_state->mode) 
    {
        case Game_Mode_Game: {
            ui_dev(render_commands, game_state, input);
            update_entities(world, game_state, input);

            draw_entities(game_state, render_commands, render_group, orthographic_group, game_state->controlling_camera->position, V3(100));

            update_console(&debug_state->console, game_state, input);
            draw_console(&debug_state->console, orthographic_group, game_state->real_time, draw_width, draw_height);

            debug_update_game_speed(debug_state, input);
            draw_game_speed_text(debug_state, input, orthographic_group, &assets->menu_font);
        } break;

        case Game_Mode_Editor: {
            if (!game_state->editor_initted) 
            {
                game_state->editor_initted = true;
                update_entities(world, game_state, input);
                game_state->controlling_camera = game_state->debug_camera;
            }

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
                Os_Time time = os.get_system_time();
                char backuppath[128];
                int len = str_snprintf(backuppath, sizeof(backuppath), "map/backup/map1_%d_%d_%d_%d_%d_%d.smap", time.year, time.month, time.day, time.hour, time.minute, time.second);
                os.file_copy(utf8lit("map/map1.smap"), utf8((u8 *)backuppath, len));

                FILE *file = fopen("map/map1.smap", "wb");
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

            update_and_draw_entity_panel(game_state);
            // no update entities in editor mode.
            update_cameras(world, game_state, input);
            draw_entities(game_state, render_commands, render_group, orthographic_group, game_state->controlling_camera->position, V3(100));

            update_console(&debug_state->console, game_state, input);
            draw_console(&debug_state->console, orthographic_group, game_state->real_time, draw_width, draw_height);
        } break;

        case Game_Mode_Menu: {

        } break;

        INVALID_DEFAULT_CASE;
    }

    debug_draw_performance(orthographic_group, input, &assets->times);

    { // @Note: Render Commands
        render_commands->time = game_state->game_time;

        render_commands->input = *input;

        render_commands->main_eye_position = game_state->controlling_camera->position;
        render_commands->main_view_proj = game_state->controlling_camera->VP;
        render_commands->ortho_view_proj = game_state->orthographic_camera->VP;

        render_commands->wireframe_color = V4(0.9f, 0.9f, 0.9f, 1.0f);

        { // @Note: Skybox
            render_commands->skybox_on = true;
            render_commands->skybox_mesh = &assets->skybox_mesh;
            render_commands->skybox_eye_view_proj = game_state->controlling_camera->VP;
            for (u32 i = 0; i < 6; ++i) {
                render_commands->skybox_textures[i] = assets->skybox_textures + i;
            }
        }

        { // @Note: CSM
            render_commands->csm_to_light = normalize(V3(1,1,1));
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
            for (u32 i = 0; i < 4; ++i) {
                positions[i] = inv * ndcs[i];
                positions[i].xyz *= (1.0f / positions[i].w);
            }
            for (u32 i = 0; i < 4; ++i) {
                v3 d = normalize(positions[i].xyz - eye);
                positions[4+i] = positions[i];
                positions[4+i].xyz += (csm_frustum_edge_length*d);
            }
            for (u32 i = 0; i < 8; ++i) {
                render_commands->csm_frustum_positions[i] = positions[i].xyz;
            }
            render_commands->csm_view = game_state->game_camera->V;
        }
    }

    ui.end_frame();
}
