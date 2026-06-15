// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RHI_GL_H
#define RTS_RHI_GL_H


#include "renderer/opengl/gl.h"


#if OS_WINDOWS
#  include "./rhi_gl_win32.h"
#else 
#  error UNDEFINED_OS
#endif


internal bool rhi_init(RHI_State* rhi, OS_Handle window);

#endif // RTS_RHI_GL_H
