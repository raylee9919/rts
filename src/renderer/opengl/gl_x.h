// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

// Error on undefined OpenGL version.
//
#ifndef RTS_GL_VERSION_MAJOR
#  error "OpenGL version major is not defined."
#endif

#ifndef RTS_GL_VERSION_MINOR
#  error "OpenGL version minor is not defined."
#endif


#define GL_FUNCTIONS(X) \
    X(PFNGLGETSTRINGIPROC,                      glGetStringi) \
    X(PFNGLUNIFORM2UIPROC,                      glUniform2ui) \
    X(PFNGLCREATESHADERPROC,                    glCreateShader) \
    X(PFNGLTEXTUREPARAMETERFPROC,               glTextureParameterf) \
    X(PFNGLTEXTURESUBIMAGE3DPROC,               glTextureSubImage3D) \
    X(PFNGLTEXTURESTORAGE3DPROC,                glTextureStorage3D) \
    X(PFNGLTEXTUREPARAMETERFVPROC,              glTextureParameterfv) \
    X(PFNGLBINDTEXTUREUNITPROC,                 glBindTextureUnit) \
    X(PFNGLCREATEFRAMEBUFFERSPROC,              glCreateFramebuffers) \
    X(PFNGLNAMEDFRAMEBUFFERTEXTUREPROC,         glNamedFramebufferTexture) \
    X(PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC,     glCheckNamedFramebufferStatus) \
    X(PFNGLCLEARNAMEDFRAMEBUFFERFVPROC,         glClearNamedFramebufferfv) \
    X(PFNGLCREATEBUFFERSPROC,                   glCreateBuffers) \
    X(PFNGLNAMEDBUFFERDATAPROC,                 glNamedBufferData) \
    X(PFNGLNAMEDBUFFERSTORAGEPROC,              glNamedBufferStorage) \
    X(PFNGLNAMEDBUFFERSUBDATAPROC,              glNamedBufferSubData) \
    X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,         glVertexArrayVertexBuffer) \
    X(PFNGLVERTEXARRAYELEMENTBUFFERPROC,        glVertexArrayElementBuffer) \
    X(PFNGLENABLEVERTEXARRAYATTRIBPROC,         glEnableVertexArrayAttrib) \
    X(PFNGLDISABLEVERTEXARRAYATTRIBPROC,        glDisableVertexArrayAttrib) \
    X(PFNGLVERTEXARRAYATTRIBFORMATPROC,         glVertexArrayAttribFormat) \
    X(PFNGLVERTEXARRAYATTRIBIFORMATPROC,        glVertexArrayAttribIFormat) \
    X(PFNGLVERTEXARRAYATTRIBBINDINGPROC,        glVertexArrayAttribBinding) \
    X(PFNGLDRAWELEMENTSBASEVERTEXPROC,          glDrawElementsBaseVertex) \
    X(PFNGLBLITNAMEDFRAMEBUFFERPROC,            glBlitNamedFramebuffer) \
    X(PFNGLGETTEXTUREHANDLEARBPROC,             glGetTextureHandleARB) \
    X(PFNGLMAKETEXTUREHANDLERESIDENTARBPROC,    glMakeTextureHandleResidentARB) \
    X(PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC, glMakeTextureHandleNonResidentARB) \
    X(PFNGLSHADERSOURCEPROC,                    glShaderSource) \
    X(PFNGLCOMPILESHADERPROC,                   glCompileShader) \
    X(PFNGLCREATEPROGRAMPROC,                   glCreateProgram) \
    X(PFNGLATTACHSHADERPROC,                    glAttachShader) \
    X(PFNGLLINKPROGRAMPROC,                     glLinkProgram) \
    X(PFNGLGETPROGRAMIVPROC,                    glGetProgramiv) \
    X(PFNGLGETSHADERINFOLOGPROC,                glGetShaderInfoLog) \
    X(PFNGLVALIDATEPROGRAMPROC,                 glValidateProgram) \
    X(PFNGLGETPROGRAMINFOLOGPROC,               glGetProgramInfoLog) \
    X(PFNGLGENBUFFERSPROC,                      glGenBuffers) \
    X(PFNGLBINDBUFFERPROC,                      glBindBuffer) \
    X(PFNGLUNIFORMMATRIX4FVPROC,                glUniformMatrix4fv) \
    X(PFNGLGETUNIFORMLOCATIONPROC,              glGetUniformLocation) \
    X(PFNGLUSEPROGRAMPROC,                      glUseProgram) \
    X(PFNGLUNIFORM1IPROC,                       glUniform1i) \
    X(PFNGLBUFFERDATAPROC,                      glBufferData) \
    X(PFNGLVERTEXATTRIBPOINTERPROC,             glVertexAttribPointer) \
    X(PFNGLGETATTRIBLOCATIONPROC,               glGetAttribLocation) \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC,         glEnableVertexAttribArray) \
    X(PFNGLGENVERTEXARRAYSPROC,                 glGenVertexArrays) \
    X(PFNGLBINDVERTEXARRAYPROC,                 glBindVertexArray) \
    X(PFNGLBINDATTRIBLOCATIONPROC,              glBindAttribLocation) \
    X(PFNGLDEBUGMESSAGECALLBACKARBPROC,         glDebugMessageCallbackARB) \
    X(PFNGLDISABLEVERTEXATTRIBARRAYPROC,        glDisableVertexAttribArray) \
    X(PFNGLUNIFORM3FVPROC,                      glUniform3fv) \
    X(PFNGLVERTEXATTRIBIPOINTERPROC,            glVertexAttribIPointer) \
    X(PFNGLUNIFORM4FVPROC,                      glUniform4fv) \
    X(PFNGLVERTEXATTRIBDIVISORPROC,             glVertexAttribDivisor) \
    X(PFNGLDRAWELEMENTSINSTANCEDPROC,           glDrawElementsInstanced) \
    X(PFNGLUNIFORM1FPROC,                       glUniform1f) \
    X(PFNGLUNIFORM1FVPROC,                      glUniform1fv) \
    X(PFNGLTEXSUBIMAGE3DPROC,                   glTexSubImage3D) \
    X(PFNGLBINDIMAGETEXTUREPROC,                glBindImageTexture) \
    X(PFNGLCLEARTEXIMAGEPROC,                   glClearTexImage) \
    X(PFNGLDRAWBUFFERSPROC,                     glDrawBuffers) \
    X(PFNGLACTIVETEXTUREPROC,                   glActiveTexture) \
    X(PFNGLBINDRENDERBUFFERPROC,                glBindRenderbuffer) \
    X(PFNGLRENDERBUFFERSTORAGEPROC,             glRenderbufferStorage) \
    X(PFNGLFRAMEBUFFERRENDERBUFFERPROC,         glFramebufferRenderbuffer) \
    X(PFNGLGENRENDERBUFFERSPROC,                glGenRenderbuffers) \
    X(PFNGLBUFFERSUBDATAPROC,                   glBufferSubData) \
    X(PFNGLBUFFERSTORAGEPROC,                   glBufferStorage) \
    X(PFNGLBINDBUFFERBASEPROC,                  glBindBufferBase) \
    X(PFNGLGETBUFFERSUBDATAPROC,                glGetBufferSubData) \
    X(PFNGLTEXBUFFERPROC,                       glTexBuffer) \
    X(PFNGLUNIFORM1UIPROC,                      glUniform1ui) \
    X(PFNGLDISPATCHCOMPUTEPROC,                 glDispatchCompute) \
    X(PFNGLMEMORYBARRIERPROC,                   glMemoryBarrier) \
    X(PFNGLMAPBUFFERRANGEPROC,                  glMapBufferRange) \
    X(PFNGLUNMAPBUFFERPROC,                     glUnmapBuffer) \
    X(PFNGLGETINTEGERI_VPROC,                   glGetIntegeri_v) \
    X(PFNGLDELETEBUFFERSPROC,                   glDeleteBuffers) \
    X(PFNGLCLEARNAMEDBUFFERDATAPROC,            glClearNamedBufferData) \
    X(PFNGLDELETEFRAMEBUFFERSPROC,              glDeleteFramebuffers) \
    X(PFNGLDELETERENDERBUFFERSPROC,             glDeleteRenderbuffers) \
    X(PFNGLGENERATETEXTUREMIPMAPPROC,           glGenerateTextureMipmap) \
    X(PFNGLPATCHPARAMETERIPROC,                 glPatchParameteri) \
    X(PFNGLDELETESHADERPROC,                    glDeleteShader) \
    X(PFNGLFRAMEBUFFERTEXTUREPROC,              glFramebufferTexture) \
    X(PFNGLTEXIMAGE3DPROC,                      glTexImage3D) \
    X(PFNGLCHECKFRAMEBUFFERSTATUSPROC,          glCheckFramebufferStatus) \
    X(PFNGLUNIFORM4FPROC,                       glUniform4f) \
    X(PFNGLUNIFORM2FPROC,                       glUniform2f) \
    X(PFNGLGETUNIFORMIVPROC,                    glGetUniformiv) \
    X(PFNGLGETACTIVEUNIFORMPROC,                glGetActiveUniform) \
    X(PFNGLGETACTIVEATTRIBPROC,                 glGetActiveAttrib) \
    X(PFNGLDRAWARRAYSINSTANCEDPROC,             glDrawArraysInstanced) \
    X(PFNGLGETSHADERIVPROC,                     glGetShaderiv) \
    X(PFNGLCREATETEXTURESPROC,                  glCreateTextures) \
    X(PFNGLTEXTUREPARAMETERIPROC,               glTextureParameteri) \
    X(PFNGLTEXTURESTORAGE2DPROC,                glTextureStorage2D) \
    X(PFNGLTEXTURESUBIMAGE2DPROC,               glTextureSubImage2D) \
    X(PFNGLBINDFRAMEBUFFERPROC,                 glBindFramebuffer) \
    X(PFNGLFRAMEBUFFERTEXTURE2DPROC,            glFramebufferTexture2D) \
    X(PFNGLDEBUGMESSAGECONTROLPROC,             glDebugMessageControl)



#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X
