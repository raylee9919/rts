// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_PASS_GEOMETRY_H
#define RTS_PASS_GEOMETRY_H


#include "renderer/renderer.h"

struct R_Pass_Geometry : R_Pass
{
    #include "shaders/pass/surface.h"

    RHI_Buffer           arguments_buffer;
    RHI_Buffer_View      arguments_view;
    void                *arguments_ptr;
};

R_PASS_DECLARE_PROCS( Geometry );


#endif // RTS_PASS_GEOMETRY_H
