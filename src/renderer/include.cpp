// Copyright Seong Woo Lee. All Rights Reserved.

global f32 VIEWPORT_WIDTH      = 1920.f;
global f32 VIEWPORT_HEIGHT     = 1080.f;
global Guid                 pipeline;
global Guid                 cube_mesh;
global RHI_Buffer          arguments_buffer;
global RHI_Buffer_View     arguments_view;
global void                *arguments_ptr;
global RHI_Buffer          material_buffer;
global RHI_Buffer_View     material_view;
global void                *material_ptr;
global Guid                 doggo_guid;
global RHI_Buffer          camera_buffer;
global RHI_Buffer_View     camera_view;
global void                *camera_ptr;

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

// @Todo: Broken... turn off vsync and you'll see.
void r_render(Game_State *g, f64 refresh_dt) 
{
    ProfileScope;

    // Render tick
    f64 dt = 0.0;
    {
        if (last_timestamp != 0.f) {
            f64 new_timestamp = last_timestamp + refresh_dt;
            dt = new_timestamp - g->time;
            game_tick(g, dt);
            last_timestamp = new_timestamp;
        } else {
            last_timestamp = g->time;
        }
    }


    gfx_pass_begin(RENDER_PASS_GEOMETRY);
    {
        gfx_pass_color_attachment(RENDER_PASS_GEOMETRY, 0, GFX_SURFACE_TEXTURE);
        gfx_pass_clear_color(RENDER_PASS_GEOMETRY, 0xff201010, 0);

        gfx_pass_depth_attachment(RENDER_PASS_GEOMETRY, gfx->depth_textures[gfx->current_frame % gfx->info.num_frames]);
        gfx_pass_clear_depth(RENDER_PASS_GEOMETRY, 1.f);

        u32 w = gfx->info.width;
        u32 h = gfx->info.height;
        gfx_set_viewport(0.f, 0.f, w, h);
        gfx_set_scissor(0, 0, w, h);

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
                    c.arguments_index = i;
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

    gfx_end(g->time, 1); // @Temporary
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

        init.num_buffers            = 3;
        init.num_frames             = 2;

        gfx_init(init);
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
