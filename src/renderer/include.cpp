// Copyright Seong Woo Lee. All Rights Reserved.

void render(f64 dt, u32 sync_interval)
{
    gfx_pass_begin(RENDER_PASS_GEOMETRY);
    {
#if 0
        gfx_pass_color_attachment(RENDER_PASS_GEOMETRY, 0, GFX_SURFACE_TEXTURE);
        gfx_pass_clear_color(RENDER_PASS_GEOMETRY, 0xff201010, 0);

        gfx_pass_depth_attachment(RENDER_PASS_GEOMETRY, gfx->depth_textures[gfx->current_frame % gfx->info.num_frames]);
        gfx_pass_clear_depth(RENDER_PASS_GEOMETRY, 1.f);

        u32 w = resolutions[resolution_index][0];
        u32 h = resolutions[resolution_index][1];
        gfx_set_viewport(0.f, 0.f, w, h);
        gfx_set_scissor(0, 0, w, h);

        gfx_set_pipeline(pipeline);

        {
            // @Temporary
            auto *mesh = table_find_pointer(&gfx->mesh_table, cube_mesh);

            // Bind root constants
            Constants c = {};
            c.vertex_buffer_id   = mesh->vertex_buffer_view.bindless;
            c.linear_sampler_id  = gfx->linear_sampler.bindless;
            c.camera_id          = camera_view.bindless;
            c.arguments_id       = arguments_view.bindless;
            c.material_buffer_id = material_view.bindless;

            gfx_push_constants(&c, sizeof(c));

            {
                auto *state = &game_state;

                for (u32 i = 0; i < NUM_ENTITIES; ++i) {
                    auto *E = &state->entities[i];

                    Arguments *args = (Arguments *)arguments_ptr + i;

                    m4x4 m = m4x4_translate(E->position) * y_rotation(state->time);
                    memcpy(&args->transform, &m, sizeof(args->transform));

                    GFX_Material *mat = gfx_material_pointer_from_guid(E->material);
                    GPU_Material sm   = gpu_material_from_gfx(mat);
                    GPU_Material *dst = (GPU_Material *)material_ptr + i;
                    memcpy(dst, &sm, sizeof(sm));
                    args->material_id = i;
                }

                GPU_Camera gpu_camera = gpu_camera_from_game(&state->camera);
                memcpy(camera_ptr, &gpu_camera, sizeof(gpu_camera));
            }

            gfx_draw(cube_mesh, NUM_ENTITIES);
        }
#endif
    }
    gfx_pass_end();

    gfx_end(dt, sync_interval);
}

void render_entry(void *param) 
{
    thread_set_name(S("Render Thread"));

    { // Init
        GFX_Info init = {};
        init.kind                   = RHI_KIND_D3D12;
#if BUILD_DEBUG
        init.debug                  = true;
        init.break_on_warning       = true;
#endif
        init.native_window_handle   = param;
        init.width                  = 1920; // @Temporaru
        init.height                 = 1080;

        init.num_buffers            = 3;
        init.num_frames             = 2;

        gfx_init(init);
    }

    {
        Construct(&render_queue);
        semaphore_create(&render_queue.semaphore);
    }

    // Loop
    while (!gfx->should_shutdown) {
        ProfileScopeN("Render Thread Loop");

        {
            ProfileScopeN("SleepOnRender_ITC_SPSC_Queue");
            semaphore_wait(&render_queue.semaphore, -1);
        }
        Render_Entry entry = render_queue.pop();

        clear_temporary_storage();
    }

    // Cleanup
    gfx_shutdown();
}
