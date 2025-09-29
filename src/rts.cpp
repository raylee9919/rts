/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// # Todo: We are testing our metaprogramming currently. Those defines are kind of 
//         API for serializing data types of entity known to serialization module.
#define BEGIN_ENTITY
#define END_ENTITY

// # Note: [.h]
//
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_random.h"
#include "rts_platform.h"
#include "rts_asset.h"
#include "rts_ds.h"
#include "ui/rts_ui_inc.h"
#include "rts_delaunay.h"
#include "rts_nav.h"
#include "rts.h"
#include "rts_geogen.h"
#include "renderer/rts_renderer.h"
#include "generated/entity.h"
#include "generated/entity_serialization.h"
#include "rts_map_loader.h"
#include "rts_sim.h"

// # Note: globals.
//
global Ui_State *ui_state;
global Renderer *renderer;


// # Note: [.cpp]
//
#include "base/rts_base_inc.cpp"
#include "rts_random.cpp"
#include "rts_asset.cpp"
#include "renderer/rts_renderer.cpp"
#include "rts_geogen.cpp"
#include "ui/rts_ui_inc.cpp"
#include "rts_delaunay.cpp"
#include "rts_nav.cpp"
#include "rts_sim.cpp"
#include "rts_map_loader.cpp"


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

#if 0
internal void
ui_dev(Render_Commands *render_commands, Game_State *game_state, Input *input)
{
    v4 color = V4(0.2f,0.2f,0.4f,0.6f);
    ui.begin("Dev", V2(-0.98f, 0.7f));
    ui.checkbox(&render_commands->wireframe_mode, color, "Wireframe", "Draw meshes' wireframe.");
    ui.checkbox(&render_commands->draw_navmesh, color, "Navmesh", "Draw Navigation Mesh.");
    ui.checkbox(&render_commands->draw_csm_frustum, color, "CSM Frustum", "Draw frustum volume for cascaded shadow mapping.");
    ui.checkbox(&render_commands->csm_varient_method, color, "CSM Valient's Method", "Use Valient's algorithm introduced in Shaderx book.");
    if (ui.button(color, "Switch Camera", "Switch between game camera and debug camera.")) 
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
    ui.end();
}
#endif




struct Stream
{
    u8 *ptr;
};

#define stream_eat_type(stream, type) (*((type *)stream_eat(stream, sizeof(type))))
internal void *
stream_eat(Stream *stream, u64 size)
{
    void *result = stream->ptr;
    stream->ptr += size;
    return result;
}

#define stream_peek_type(stream, type) (*((type *)stream_peek(stream)))
internal void *
stream_peek(Stream *stream)
{
    void *result = stream->ptr;
    return result;
}

// # Temporary:
//
#include <unordered_map>

struct Glyph_Metrics
{
    u32 glyph_index;
    f32 uv_min_x;
    f32 uv_min_y;
    f32 uv_max_x;
    f32 uv_max_y;
    f32 width;
    f32 height;
    f32 left_side_bearing;
    f32 top_side_bearing;
    f32 advance_x;
};

std::unordered_map<u32, std::vector<u16>> cmap;
std::unordered_map<u16, Glyph_Metrics> metrics_table;

internal void
render_debug_string(Render_Id atlas, Utf8 string)
{
    v2 pen = {100, 100};

    u8 *ptr = string.str;
    u8 *opl = ptr + string.len;
    u64 size = 0;
    Unicode_Decode consume = {};
    for (;ptr < opl; ptr += consume.inc)
    {
        consume = utf8_decode(ptr, opl - ptr);
        u32 codepoint = consume.codepoint;

        if (cmap.find(codepoint) == cmap.end())
        {
            codepoint = 0;
        }

        std::vector<u16> glyphs = cmap[codepoint];
        for (u16 glyph : glyphs)
        {
            Glyph_Metrics metrics = metrics_table[glyph];

            v2 offset = v2{metrics.left_side_bearing, metrics.top_side_bearing};
            v2 dim = v2{metrics.width, metrics.height};
            v2 box_origin = pen + offset;
            v2 uv_min = v2{metrics.uv_min_x, metrics.uv_min_y};
            v2 uv_max = v2{metrics.uv_max_x, metrics.uv_max_y};
            render_quad_tuv(atlas, box_origin, box_origin + dim, uv_min, uv_max);

            pen.x += floorf(metrics.advance_x);
        }
    }
}









no_name_mangle
GAME_UPDATE_AND_RENDER(game_update_and_render)
{
    // # Note: acquire os.
    //
    if (os == NULL)
    {
        os = platform->os;
    }

    // # Note: acquire renderer.
    //
    if (renderer == NULL)
    {
        renderer = platform->renderer;
        render_init();
    }

    // # Note: acquire game state.
    //
    Game_State *game_state = (Game_State *)platform->game_state;
    if (! game_state) 
    {
        platform->game_state = game_state = push_struct(platform->arena, Game_State);
    }

    {
        game_state->draw_width  = platform->draw_width;
        game_state->draw_height = platform->draw_height;
        // # Todo: Warn on out-out-range refresh.
        game_state->dt_real = clamp(platform->dt, 0.001f, 0.1f);
        game_state->dt_game = game_state->dt_real;
    }

    if (! game_state->initted) 
    {
        game_state->initted = true;

        thread_init();

        // # Note: alloc assets
        //
        Arena *arena = arena_alloc();
        game_state->assets = push_struct(arena, Game_Assets);
        game_state->assets->arena = arena;


        game_state->frame_arena = arena_alloc();

        // # Note: init world.
        //
        Arena *world_arena = arena_alloc();
        World *world = game_state->world = push_struct(world_arena, World);
        world->arena = world_arena;
        world->next_entity_id = 1;

        game_state->mode = GAME_MODE_GAME;
        game_state->random_series = rand_seed(1219);

        { // # Temporary
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

            {
                asset_load_font(asset_arena, utf8f(scratch.arena, "%S/font/Times New Roman.sfnt", platform->data_path), &assets->times);
                asset_load_font(asset_arena, utf8f(scratch.arena, "%S/font/noto_serif.sfnt", platform->data_path), &assets->debug_font);
                asset_load_font(asset_arena, utf8f(scratch.arena, "%S/font/gill_sans.sfnt", platform->data_path), &assets->menu_font);
                asset_load_font(asset_arena, utf8f(scratch.arena, "%S/font/Karmina Regular.sfnt", platform->data_path), &assets->karmina);
            }

            asset_load_image(&game_state->assets->debug_bitmap, utf8f(scratch.arena, "%S/textures/doggo.sbmp", platform->data_path), asset_arena);

            char *skybox_filenames[6] = {"right", "left", "top", "bottom", "front", "back"};
            for (u32 i = 0; i < 6; ++i) 
            {
                asset_load_image(assets->skybox_textures + i,
                                 utf8f(scratch.arena, "%S/textures/%s.sbmp", platform->data_path, skybox_filenames[i]),
                                 asset_arena);
            }

            // # Temporary: init cameras.
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

            // # Temporary: We are setting navmesh input data by hand.
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
                    push_vertex(navmesh, (transform*v4{-1.0f, 0,-1.0f, 1}).xyz);
                    push_vertex(navmesh, (transform*v4{-1.0f, 0, 1.0f, 1}).xyz);
                    push_vertex(navmesh, (transform*v4{ 1.0f, 0, 1.0f, 1}).xyz);
                    push_vertex(navmesh, (transform*v4{ 1.0f, 0,-1.0f, 1}).xyz);
                    end_constrain(navmesh);
                }
            }

            // # Todo: not neat.
            begin_constrain(navmesh);
            {
                push_vertex(navmesh, v3{ 1.0f, 0, 3.0f});
                push_vertex(navmesh, v3{ 3.0f, 0, 2.0f});
                push_vertex(navmesh, v3{ 2.0f, 0,-2.0f});
                push_vertex(navmesh, v3{-1.0f, 0,-4.0f});
                push_vertex(navmesh, v3{-3.0f, 0,-3.0f});
                push_vertex(navmesh, v3{-4.0f, 0, 1.0f});
            }
            end_constrain(navmesh);

            for (u32 i = 0; i < navmesh->vertex_count; ++i)
            {
                navmesh->vertices[i].position.y = 0.01f;
            }

            navmesh->cdt = delaunay_triangulate(navmesh->vertices, navmesh->vertex_count, navmesh);
        }
    }

    // # Note: ui alloc/init
    //
    if (ui_state == NULL)
    {
        ui_state = ui_alloc();
        ui_init(ui_state);
    }

    // # Note: this temporary frame arena must be cleared every frame.
    arena_clear(game_state->frame_arena);

    // # Note: clear renderer
    render_begin();

    // # Temporary:
    game_state->active_entity_id = render_commands->active_entity_id;
    game_state->view_proj = game_state->controlling_camera->VP;

    // # Note: Alias
    World *world = game_state->world;
    Game_Assets *assets = game_state->assets;

    Render_Group *render_group       = begin_render_group(render_commands, MB(16));
    Render_Group *orthographic_group = begin_render_group(render_commands, MB(16));

    // # Temporary:
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

    switch (game_state->mode) 
    {
        case GAME_MODE_GAME: 
        {
            // ui_dev(render_commands, game_state, input);
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

        default: { assert(! "invalid default case"); } break;
    }

    { // # Note: Render Commands
        render_commands->main_eye_position = game_state->controlling_camera->position;
        render_commands->main_view_proj = game_state->controlling_camera->VP;
        render_commands->ortho_view_proj = game_state->orthographic_camera->VP;

        render_commands->wireframe_color = V4(0.9f, 0.9f, 0.9f, 1.0f);

        // # Note: Skybox
        //
        render_commands->skybox_on = true;
        render_commands->skybox_mesh = &assets->skybox_mesh;
        render_commands->skybox_eye_view_proj = game_state->controlling_camera->VP;
        for (u32 i = 0; i < 6; ++i) {
            render_commands->skybox_textures[i] = assets->skybox_textures + i;
        }


        // # Note: CSM
        //
        render_commands->csm_to_light = normalize(V3(1,1,1));
        f32 csm_frustum_edge_length = 50.0f;
        m4x4 inv = inverse(game_state->game_camera->VP);
        // # Todo: Renderer independent calculation!
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
            positions[i].xyz *= (1.0f / positions[i].w);
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

    // # Temporary:
    //
    local_persist b32 initted = 0;
    local_persist Render_Id id = {};
    if (! initted)
    {
        initted = 1;
        Utf8 contents = read_entire_file(assets->arena, utf8f(assets->arena, "%S/font_asset.txt", platform->data_path));

        Stream stream = {};
        stream.ptr = contents.str;

        // # Note: Part1
        //
        while (stream_peek_type(&stream, u8) != 0)
        {
            // # Temporary
            std::vector<u16> indices;


            u32 codepoint = stream_eat_type(&stream, u32);
            u32 glyph_count = stream_eat_type(&stream, u32);
            for (u32 i = 0; i < glyph_count; ++i)
            {
                u16 glyph_index = stream_eat_type(&stream, u16);

                // # Temporary
                indices.push_back(glyph_index);
            }

            cmap[codepoint] = indices;
        }
        assert(stream_eat_type(&stream, u8) == 0);


        // # Note: Part2
        //
        while (stream_peek_type(&stream, u8) != 0)
        {
            u16 glyph_index = stream_eat_type(&stream, u32);

            Glyph_Metrics metrics = {};
            {
                metrics.uv_min_x            = stream_eat_type(&stream, f32);
                metrics.uv_min_y            = stream_eat_type(&stream, f32);
                metrics.uv_max_x            = stream_eat_type(&stream, f32);
                metrics.uv_max_y            = stream_eat_type(&stream, f32);
                metrics.width               = stream_eat_type(&stream, f32);
                metrics.height              = stream_eat_type(&stream, f32);
                metrics.left_side_bearing   = stream_eat_type(&stream, f32);
                metrics.top_side_bearing    = stream_eat_type(&stream, f32);
                metrics.advance_x           = stream_eat_type(&stream, f32);
            }

            metrics_table[glyph_index] = metrics;
        }
        assert(stream_eat_type(&stream, u8) == 0);


        // # Note: Part3
        //
        u32 atlas_width = stream_eat_type(&stream, u32);
        u32 atlas_height = stream_eat_type(&stream, u32);
        u64 atlas_size = atlas_width * atlas_height * 4;
        void *atlas_data = stream_eat(&stream, atlas_size);
        assert(stream_eat_type(&stream, u8) == 0);



        // # Note: Uplaod atlas to renderer.
        //
        id = render_texture_create_filter_dot(RENDER_TEXTURE_TYPE_R8G8B8A8, atlas_data, atlas_width, atlas_height);
    }

    render_debug_string(id, utf8lit("Hello, World!"));

    render_end();
}
