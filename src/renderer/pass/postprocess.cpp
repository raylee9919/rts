// Copyright Seong Woo Lee. All Rights Reserved.

#include "./postprocess.h"
#include "gfx/gfx.h"

R_PASS_INIT( RenderPassInit_Postprocess )
{
    auto *result = (R_Pass_Postprocess *)alloc(sizeof(R_Pass_Postprocess), renderer->heap);
    Construct( result );

    result->name    = S("Postprocess");
    result->deinit  = RenderPassDeinit_Postprocess;
    result->execute = RenderPassExecute_Postprocess;

    return result;
}

R_PASS_DEINIT( RenderPassDeinit_Postprocess )
{
    dealloc(inPass, renderer->heap);
}

R_PASS_EXECUTE( RenderPassExecute_Postprocess )
{
    u32 w = inInfo.width;
    u32 h = inInfo.height;

    GFX_Pass pass  = {};
    pass.name                 = inPass->name;
    pass.viewport             = {0.f, 0.f, (f32)w, (f32)h};
    pass.scissor              = {0, 0, w, h};
    pass.color_attachments[0] = renderer->scene[gfx_backbuffer_index()];

    gfx_pass_begin(R_PASS_POSTPROCESS, &pass);
    {
    }
    gfx_pass_end();
}
