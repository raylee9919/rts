// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RHI_GL_WIN32_H
#define RTS_RHI_GL_WIN32_H

#include "ThirdParty/opengl/wglext.h"

static PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;


#endif // RTS_RHI_GL_WIN32_H
