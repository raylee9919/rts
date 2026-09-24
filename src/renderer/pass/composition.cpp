// Copyright Seong Woo Lee. All Rights Reserved.

#include "./composition.h"
#include "shader_compiler/shader.h"
#include "gfx/gfx.h"
#include "shared.h"

R_PASS_INIT( RenderPassInit_Composition )
{
    auto *result = (R_Pass_Composition *)alloc(sizeof(R_Pass_Composition), renderer->heap);
    Construct( result );

    result->name    = S("Composition");
    result->deinit  = RenderPassDeinit_Composition;
    result->execute = RenderPassExecute_Composition;

    // @Temporary
    String path   = tprint(S("%S/%S"), shared->data_path, S("shaders/pass/composition.slang"));
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
            desc.color_attachment_formats[0] = gfx_surface_format();
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

    return result;
}

R_PASS_DEINIT( RenderPassDeinit_Composition )
{
    auto *pass = (R_Pass_Composition *)inPass;
    gfx_pipeline_destroy(pass->pipeline_id);
    dealloc(inPass, renderer->heap);
}

R_PASS_EXECUTE( RenderPassExecute_Composition )
{
    auto *pass = (R_Pass_Composition *)inPass;

    GFX_Pass gfx_pass  = {};
    gfx_pass.name                 = inPass->name;
    gfx_pass.viewport             = {inInfo.x, inInfo.y, inInfo.w, inInfo.h};
    gfx_pass.scissor              = {(u32)inInfo.x, (u32)inInfo.y, (u32)inInfo.w, (u32)inInfo.h};
    gfx_pass.color_attachments[0] = gfx_surface_texture();

    gfx_pass_begin(R_PASS_COMPOSITION, &gfx_pass);
    {
        // Set pipeline
        gfx_set_pipeline(pass->pipeline_id);

        // Push constants
        R_Pass_Composition::Push_Constants c = {};
        c.linear_sampler_id = gfx->linear_sampler.bindless;
        c.scene_texture_id  = gfx_srv_bindless_from_texture( renderer->scene_texture[gfx_backbuffer_index()]) ;
        gfx_push_constants(&c, sizeof(c));

        // Draw
        gfx_draw(renderer->fullscreen_triangle_mesh, 1);
    }
    gfx_pass_end();
}
