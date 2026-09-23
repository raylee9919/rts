// Copyright Seong Woo Lee. All Rights Reserved.

#include "./geometry.h"
#include "gfx/gfx.h"
#include "game.h"
#include "material/material.h"
#include "animation/animation.h"

#define R_MAX_SKINNING_MATRICES 65536 // @Temporary

R_PASS_INIT( RenderPassInit_Geometry )
{
    auto *result = (R_Pass_Geometry *)alloc(sizeof(R_Pass_Geometry), renderer->heap);
    Construct( result );

    result->name    = S("Geometry");
    result->deinit  = RenderPassDeinit_Geometry;
    result->execute = RenderPassExecute_Geometry;

    { // Create arguments buffer and view
        u64 stride = sizeof(R_Pass_Geometry::Arguments);
        u64 sz     = stride * 16777216; // @Temporary

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        R_ASSERT(rhi_buffer_init(gfx->device, &result->arguments_buffer, &desc, NULL));

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = stride;
            view_desc.offset   = 0;
            view_desc.size     = sz;
        }

        rhi_buffer_view_init(gfx->device, &result->arguments_view, &result->arguments_buffer, &view_desc);

        result->arguments_ptr = rhi_buffer_map(&result->arguments_buffer);
    }

    { // Create skinning buffer and view
        u64 stride = sizeof(m3x4);
        u64 sz     = stride * R_MAX_SKINNING_MATRICES;

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        R_ASSERT(rhi_buffer_init(gfx->device, &result->skinning_buffer, &desc, NULL));

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = stride;
            view_desc.offset   = 0;
            view_desc.size     = sz;
        }

        rhi_buffer_view_init(gfx->device, &result->skinning_view, &result->skinning_buffer, &view_desc);

        result->skinning_ptr = rhi_buffer_map(&result->skinning_buffer);
    }

    return result;
}

R_PASS_DEINIT( RenderPassDeinit_Geometry )
{
    dealloc(inPass, renderer->heap);
}

R_PASS_EXECUTE( RenderPassExecute_Geometry )
{
    auto *pass = (R_Pass_Geometry *)inPass;

    u32 w = inInfo.width;
    u32 h = inInfo.height;

    Game_State *g = inInfo.game_state;

    GFX_Pass gfx_pass  = {};
    gfx_pass.name                 = inPass->name;
    gfx_pass.viewport             = {0.f, 0.f, (f32)w, (f32)h};
    gfx_pass.scissor              = {0, 0, w, h};
    gfx_pass.color_attachments[0] = renderer->gbuffer_color[gfx_backbuffer_index()];
    gfx_pass.depth_attachment     = renderer->scene_depth[gfx_backbuffer_index()];
    gfx_pass.min_depth            = 0.f;
    gfx_pass.max_depth            = 1.f;

    // @Todo: Do something about this enum
    // @Cleanup: Proper buffers!
    pass->skinning_used = 0;

    gfx_pass_begin(R_PASS_GEOMETRY, &gfx_pass);
    {
        entity_dfs(g, g->root, pass, [](Game_State *g, Entity *E, u64 i, void *data) {
            // Set pipeline
            M_Entry *material = material_from_guid(E->material);
            gfx_set_pipeline(material->pipeline);


            // Upload arguments
            auto *p = (R_Pass_Geometry*)data;

            auto *args = (R_Pass_Geometry::Arguments*)p->arguments_ptr + i;

            m4x4 m = m4x4_translate(E->position)
                   * y_rotation((f32)g->time)
                   * m4x4_scale(E->scale.x, E->scale.y, E->scale.z);
            memcpy(&args->transform, &m, sizeof(args->transform));
            args->material_index = material->offset;

            // Copy the skinning matrices
            args->skinning_base_index = 0;
            args->num_joints          = 0;

            Animation_Player *player = animation_player_from_offset(g, E->animation_player);
            if (player) {
                u32 num_joints = player->skeleton->num_joints;
                R_ASSERT(p->skinning_used + num_joints <= R_MAX_SKINNING_MATRICES);

                memcpy((m3x4 *)p->skinning_ptr + p->skinning_used,
                       animation_player_skinning_matrices(g, player),
                       sizeof(m3x4) * num_joints);

                args->skinning_base_index = p->skinning_used;
                args->num_joints          = num_joints;
                p->skinning_used         += num_joints;
            }


            GFX_Mesh *mesh = table_find_pointer(&gfx->mesh_table, E->mesh);
            if (mesh) {
                // Push constants
                R_Pass_Geometry::Push_Constants c = {};
                c.vertex_buffer_id    = mesh->vertex_buffer_view.bindless;
                c.linear_sampler_id   = gfx->linear_sampler.bindless;
                c.camera_buffer_id    = renderer->camera_view.bindless;
                c.argument_buffer_id  = p->arguments_view.bindless;
                c.argument_base_index = i;
                c.material_buffer_id  = renderer->material_buffer.view.bindless;
                c.skinning_buffer_id  = p->skinning_view.bindless;
                gfx_push_constants(&c, sizeof(c));

                // Draw
                gfx_draw(E->mesh, 1);
            }
        });


        // Upload camera
        GPU_Camera gpu_camera = gpu_camera_from_game(&g->camera);
        memcpy(renderer->camera_ptr, &gpu_camera, sizeof(gpu_camera));
    }
    gfx_pass_end();
}
