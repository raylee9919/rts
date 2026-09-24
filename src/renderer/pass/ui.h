// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PASS_UI_H
#define RTS_PASS_UI_H

#include "renderer/renderer.h"

struct R_Pass_UI : R_Pass
{
    #include "shaders/pass/ui.h"

    Guid pipeline_id;

    Guid   quad_mesh;
    f32    quad_vertices[4];
    u32    quad_indices[6];


    RHI_Buffer           quad_buffer;
    RHI_Buffer_View      quad_view;
    u8                  *quad_ptr;
};

R_PASS_DECLARE_PROCS( UI );


#endif // RTS_PASS_UI_H
