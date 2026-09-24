// Copyright Seong Woo Lee. All Rights Reserved.

#include "./ui.h"
#include "shared.h"
#include "shader_compiler/shader.h"
#include "renderer/draw.h"

#define R_MAX_QUADS 2048

R_PASS_INIT( RenderPassInit_UI )
{
    auto *result = (R_Pass_UI *)alloc(sizeof(R_Pass_UI), renderer->heap);
    Construct( result );

    result->name    = S("UI");
    result->deinit  = RenderPassDeinit_UI;
    result->execute = RenderPassExecute_UI;

    { // Create quad buffers.
        u64 stride = sizeof(R_Pass_UI::Quad);
        u64 sz     = stride * R_MAX_QUADS * RHI_MAX_BUFFER_COUNT;

        RHI_Buffer_Desc desc = {};
        desc.memory_type = RHI_MEMORY_UPLOAD;
        desc.size        = sz;

        R_ASSERT(rhi_buffer_init(gfx->device, &result->quad_buffer, &desc, NULL));

        RHI_Buffer_View_Desc view_desc = {};
        {
            view_desc.type     = RHI_BUFFER_VIEW_TYPE_STRUCTURED;
            view_desc.writable = false;
            view_desc.stride   = stride;
            view_desc.offset   = 0;
            view_desc.size     = sz;
        }

        rhi_buffer_view_init(gfx->device, &result->quad_view, &result->quad_buffer, &view_desc);

        result->quad_ptr = (u8*)rhi_buffer_map(&result->quad_buffer);
    }

    // @Temporary
    String path   = S("shaders/pass/ui.slang");
    String source = read_entire_file(path, tctx.temp);

    Shader_Compile_Result vs = {};
    Shader_Compile_Result ps = {};
    {
        Shader_Compile_Options opts = {};
        opts.source = source;
        opts.path   = path;

        opts.stage = SHADER_STAGE_VS;
        R_ASSERT(shader_compile(shared->shader_compiler, opts, true, &vs, tctx.temp));

        opts.stage = SHADER_STAGE_PS;
        R_ASSERT(shader_compile(shared->shader_compiler, opts, true, &ps, tctx.temp));
    }

    result->pipeline_id = guid_generate();
    RHI_Pipeline_Desc desc = {};
    {
        desc.type                           = RHI_PIPELINE_TYPE_GRAPHICS;

        desc.depth_enabled                  = false;

        desc.num_color_attachments          = 1;
        {
            desc.color_attachment_formats[0] = RHI_FORMAT_RGBA8_UNORM_SRGB;
            desc.blend_enabled[0]            = false;
        }

        desc.fill_mode                      = RHI_FILL_SOLID;
        desc.cull_mode                      = RHI_CULL_NONE;

        desc.topology                       = RHI_TOPOLOGY_TRIANGLES;

        desc.vs_data                        = vs.data;
        desc.vs_size                        = vs.size;

        desc.ps_data                        = ps.data;
        desc.ps_size                        = ps.size;
    }

    gfx_pipeline_create(result->pipeline_id, desc);


    // Create quad mesh
    result->quad_mesh = guid_generate();

    result->quad_indices[0] = 0;
    result->quad_indices[1] = 1;
    result->quad_indices[2] = 2;
    result->quad_indices[3] = 2;
    result->quad_indices[4] = 1;
    result->quad_indices[5] = 3;

    gfx_mesh_create(result->quad_mesh, 
                    result->quad_vertices, 4, sizeof(f32), 
                    result->quad_indices, 6, sizeof(u32));

    return result;
}

R_PASS_DEINIT( RenderPassDeinit_UI )
{
    R_Pass_UI *pass = (R_Pass_UI *)inPass;
    gfx_pipeline_destroy(pass->pipeline_id);
    gfx_mesh_destroy(pass->quad_mesh);

    dealloc(pass, renderer->heap);
}

R_PASS_EXECUTE( RenderPassExecute_UI )
{
    R_Pass_UI *pass = (R_Pass_UI *)inPass;

    u32 res_x = inInfo.resolution_x;
    u32 res_y = inInfo.resolution_y;

    GFX_Pass gfx_pass  = {};
    gfx_pass.name                 = inPass->name;
    gfx_pass.viewport             = {0.f, 0.f, (f32)res_x, (f32)res_y};
    gfx_pass.scissor              = {0, 0, res_x, res_y};
    gfx_pass.color_attachments[0] = renderer->ui_texture[gfx_backbuffer_index()];

    gfx_pass_begin(R_PASS_UI, &gfx_pass);
    {
        gfx_set_pipeline(pass->pipeline_id);

        u32 base_index = gfx_backbuffer_index() * R_MAX_QUADS;

        R_Pass_UI::Push_Constants pc = {};
        pc.linear_sampler_id = gfx->linear_sampler.bindless;
        pc.buffer_id         = pass->quad_view.bindless;
        pc.base_index        = base_index;

        // Projection
        pc.width  = res_x;
        pc.height = res_y;


        // Fill in the buffer
        u32 N = min((u64)R_MAX_QUADS, immediate_quads.count);
        for (u32 i = 0; i < N; ++i) {

            R_Quad quad = immediate_quads.data[i];
            auto *dst = (R_Pass_UI::Quad*)pass->quad_ptr + base_index + i;
            dst->texture_id = GFX_INVALID_BINDLESS;

            for (int v = 0; v < 4; ++v) {
                dst->position[v].x = quad.vertices[v].x;
                dst->position[v].y = quad.vertices[v].y;
                dst->uv[v].x = quad.uvs[v].x;
                dst->uv[v].y = quad.uvs[v].y;

                dst->color[v].x = quad.colors[v].x;
                dst->color[v].y = quad.colors[v].y;
                dst->color[v].z = quad.colors[v].z;
                dst->color[v].w = quad.colors[v].w;
            }
        }


        // Draw N instances
        if (N > 0) {
            gfx_push_constants(&pc, sizeof(pc));
            gfx_draw(pass->quad_mesh, N);
        }
    }
    gfx_pass_end();
}
