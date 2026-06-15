// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RHI_H
#define RTS_RHI_H


struct RHI_State {
    Arena* arena;
    void*  platform;
};


internal bool rhi_init(RHI_State* rhi, OS_Window window);
internal void r_begin(RHI_State* rhi, v2 window_size, v2 render_size);
internal void r_end(RHI_State* rhi, Renderer* r);


#include "./gl/rhi_gl.h"


#endif // RTS_RHI_H
