// Copyright Seong Woo Lee. All Rights Reserved.

#include "renderer/renderer.h"
#include "basic/context.h"
#include "basic/hash_table.h"
#include "basic/log.h"
#include "gfx/gfx.h"
#include "math/math.h"
#include "os/os.h"
#include "profiler/profiler.h"
#include "shaders/shared.h"
#include "game.h"

Render_SPSC_Queue render_queue;

global f32 VIEWPORT_WIDTH      = 1920.f;
global f32 VIEWPORT_HEIGHT     = 1080.f;
Guid                 pipeline;
Guid                 cube_mesh;
RHI_Buffer           arguments_buffer;
RHI_Buffer_View      arguments_view;
void                *arguments_ptr;
RHI_Buffer           material_buffer;
RHI_Buffer_View      material_view;
void                *material_ptr;
Guid                 doggo_guid;
RHI_Buffer           camera_buffer;
RHI_Buffer_View      camera_view;
void                *camera_ptr;
Guid                 color_gbuffer;
Guid                 scene_buffer;


global f64 last_timestamp = 0.f;

void game_tick(Game_State *g, f64 dt);

GPU_Camera gpu_camera_from_game(Camera *camera)
{
    GPU_Camera result = {};

    f32 fov = pi32 * 0.5f;
    f32 aspect_ratio = (f32)gfx->info.width / (f32)gfx->info.height;
    v3 dir = (y_rotation(camera->yaw) * x_rotation(camera->pitch) * FORWARD_VECTOR).xyz;

    result.position  = V4(camera->position, 1.f);
    result.view      = look_to_rh(camera->position, dir, WORLD_UP);
    result.proj      = persp_fov_rh(fov, aspect_ratio, NEAR_Z, FAR_Z);
    result.view_proj = result.proj * result.view;

    return result;
}

static void geometry_pass(Game_State *g)
{
    u32 w = gfx->info.width;
    u32 h = gfx->info.height;

    GFX_Pass pass  = {};
    pass.name                 = S("Gemoetry Pass");
    pass.viewport             = {0.f, 0.f, (f32)w, (f32)h};
    pass.scissor              = {0, 0, w, h};
    pass.color_attachments[0] = color_gbuffer;
    pass.depth_attachment     = gfx->depth_textures[gfx_backbuffer_index()];
    pass.min_depth            = 0.f;
    pass.max_depth            = 1.f;

    gfx_pass_begin(R_PASS_GEOMETRY, &pass);
    {
        gfx_set_pipeline(pipeline);
        {
            // @Temporary

            entity_dfs(g, g->root, [](Game_State *g, Entity *E, u64 i) {
                // Upload arguments
                Arguments *args = (Arguments *)arguments_ptr + i;
                m4x4 m = m4x4_translate(E->position) * y_rotation(g->time);
                memcpy(&args->transform, &m, sizeof(args->transform));

                // Upload material
                GFX_Material *mat = gfx_material_pointer_from_guid(E->material);
                GPU_Material sm   = gpu_material_from_gfx(mat);
                GPU_Material *dst = (GPU_Material *)material_ptr + i;
                memcpy(dst, &sm, sizeof(sm));
                args->material_id = i;

                // Upload constants
                auto *mesh = table_find_pointer(&gfx->mesh_table, E->mesh);

                if (mesh) {
                    Constants c = {};
                    c.vertex_buffer_id    = mesh->vertex_buffer_view.bindless;
                    c.linear_sampler_id   = gfx->linear_sampler.bindless;
                    c.camera_buffer_id    = camera_view.bindless;
                    c.arguments_buffer_id = arguments_view.bindless;
                    c.material_buffer_id  = material_view.bindless;
                    c.arguments_index     = i;
                    gfx_push_constants(&c, sizeof(c));

                    // Draw
                    gfx_draw(cube_mesh, 1);
                }
            });


            // Upload camera
            GPU_Camera gpu_camera = gpu_camera_from_game(&g->camera);
            memcpy(camera_ptr, &gpu_camera, sizeof(gpu_camera));
        }
    }
    gfx_pass_end();
}

static void postprocess_pass(Game_State *g)
{
    u32 w = gfx->info.width;
    u32 h = gfx->info.height;

    GFX_Pass pass  = {};
    pass.name                 = S("Postprocess Pass");
    pass.viewport             = {0.f, 0.f, (f32)w, (f32)h};
    pass.scissor              = {0, 0, w, h};
    pass.color_attachments[0] = scene_buffer;

    gfx_pass_begin(R_PASS_POSTPROCESS, &pass);
    {
        gfx_set_pipeline(pipeline);
        {
        }
    }
    gfx_pass_end();
}

static void composition_pass(Game_State *g)
{
    u32 w = gfx->info.width;
    u32 h = gfx->info.height;

    GFX_Pass pass  = {};
    pass.name                 = S("Composition Pass");
    pass.viewport             = {0.f, 0.f, (f32)w, (f32)h};
    pass.scissor              = {0, 0, w, h};
    pass.color_attachments[0] = gfx_frame_texture();

    gfx_pass_begin(R_PASS_COMPOSITION, &pass);
    {
        gfx_set_pipeline(pipeline);
        {
        }
    }
    gfx_pass_end();
}

void r_render(Game_State *g, f64 refresh_dt)
{
    ProfileScope;

    // Render tick
    f64 dt = 0.0;
    // {
    //     if (last_timestamp != 0.f) {
    //         f64 new_timestamp = last_timestamp + refresh_dt;
    //         dt = new_timestamp - g->time;
    //         game_tick(g, dt);
    //         last_timestamp = new_timestamp;
    //     } else {
    //         last_timestamp = g->time;
    //     }
    // }


    { // Build frame graph
        gfx_pass_connect(color_gbuffer, 
                         -1, R_PASS_GEOMETRY, 
                         RHI_RESOURCE_STATE_RENDER_TARGET);

        gfx_pass_connect(gfx->depth_textures[gfx_backbuffer_index()], 
                         -1, R_PASS_GEOMETRY, 
                         RHI_RESOURCE_STATE_DEPTH_WRITE);

        gfx_pass_connect(color_gbuffer, 
                         R_PASS_GEOMETRY, R_PASS_POSTPROCESS, 
                         RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        gfx_pass_connect(scene_buffer, 
                         R_PASS_GEOMETRY, R_PASS_POSTPROCESS, 
                         RHI_RESOURCE_STATE_RENDER_TARGET);

        gfx_pass_connect(scene_buffer, 
                         R_PASS_POSTPROCESS, R_PASS_COMPOSITION, 
                         RHI_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        gfx_pass_connect(gfx_frame_texture(), 
                         -1, R_PASS_COMPOSITION, 
                         RHI_RESOURCE_STATE_RENDER_TARGET);
    }

    geometry_pass(g);
    postprocess_pass(g);
    composition_pass(g);

    gfx_end(g->time, gfx->info.vsync_off ? 0 : 1);
}

void r_entry(void *param)
{
    thread_set_name(S("RenderThread"));

    // @Temporary
    f64 refresh_dt = 1.0 / 120.0;

    { // Init
        GFX_Info init = {};
        init.kind                   = RHI_KIND_D3D12;
#if BUILD_DEBUG
        init.debug                  = true;
        init.break_on_warning       = true;
#endif
        init.native_window_handle   = param;
        init.width                  = 1920; // @Temporary
        init.height                 = 1080;

        init.vsync_off              = false;

        init.frame_latency_waitable = true;

        gfx_init(init, 3);
    }

    { // Init renderer's resources

        { // G-Buffer: Color
            color_gbuffer = guid_generate();
            RHI_Texture_Desc desc = {};
            {
                desc.name           = S("G-Buffer: Color");
                desc.type           = RHI_TEXTURE_TYPE_2D;
                desc.format         = RHI_FORMAT_RGBA8_UNORM;
                desc.usage          = RHI_TEXTURE_USAGE_COLOR_ATTACHMENT | RHI_TEXTURE_USAGE_STORAGE;
                desc.width          = 1920; // @Temporary
                desc.height         = 1080;
                desc.mip_levels     = 1;
                desc.depth          = 1;
                desc.clear          = true;
                desc.clear_color[0] = 0.f;
                desc.clear_color[1] = 0.f;
                desc.clear_color[2] = 0.f;
                desc.clear_color[3] = 0.f;
            }
            gfx_texture_create(color_gbuffer, desc);
        }

        { // Composited
            scene_buffer = guid_generate();
            RHI_Texture_Desc desc = {};
            {
                desc.name           = S("Composited");
                desc.type           = RHI_TEXTURE_TYPE_2D;
                desc.format         = RHI_FORMAT_RGBA8_UNORM;
                desc.usage          = RHI_TEXTURE_USAGE_COLOR_ATTACHMENT | RHI_TEXTURE_USAGE_STORAGE;
                desc.width          = 1920; // @Temporary
                desc.height         = 1080;
                desc.mip_levels     = 1;
                desc.depth          = 1;
                desc.clear          = true;
                desc.clear_color[0] = 0.f;
                desc.clear_color[1] = 0.f;
                desc.clear_color[2] = 0.f;
                desc.clear_color[3] = 0.f;
            }
            gfx_texture_create(scene_buffer, desc);
        }
    }

    // Loop
    while (!gfx->should_shutdown) {
        ProfileScopeN("RenderThreadLoop");

        mutex_lock(&render_queue.mutex);

        while (render_queue.is_empty()) {
            ProfileScopeN("RenderThreadSleepUntilWorkArrives");
            condvar_sleep(&render_queue.condvar, &render_queue.mutex, -1);
        }

        Assert(!render_queue.is_empty());

        auto *rq = &render_queue;

        Render_Entry *entry = &rq->entries[rq->read_idx];
        rq->read_idx = (rq->read_idx + 1) % array_count(rq->entries);

        { mutex_lock(&entry->mutex);

            condvar_wake_all(&rq->condvar);
            mutex_unlock(&render_queue.mutex);

            r_render(entry->game_state, refresh_dt);

        } mutex_unlock(&entry->mutex);

        clear_thread_temporary_storage();
    }

    // Cleanup
    gfx_shutdown();

    log(LOG_INFO, S("Render thread returned successfully."));
}
