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


// ARB
//
typedef void (APIENTRY  *GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);

// @Todo: This was a bad idea. X macro is much cleaner...
// Scrapped from OpenGL core header file.
//
typedef BOOL        Type_wglSwapIntervalEXT(int interval);
typedef GLuint      Type_glCreateShader(GLenum shaderType);
typedef void        Type_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef void        Type_glCompileShader(GLuint shader);
typedef GLuint      Type_glCreateProgram(void);
typedef void        Type_glAttachShader(GLuint program, GLuint shader);
typedef void        Type_glLinkProgram(GLuint program);
typedef void        Type_glGetProgramiv(GLuint program, GLenum pname, GLint *params);
typedef void        Type_glGetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void        Type_glValidateProgram(GLuint program);
typedef void        Type_glGetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef void        Type_glGenBuffers(GLsizei n, GLuint *buffers);
typedef void        Type_glBindBuffer(GLenum target, GLuint buffer);
typedef void        Type_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef GLint       Type_glGetUniformLocation(GLuint program, const GLchar *name);
typedef void        Type_glUseProgram(GLuint program);
typedef void        Type_glUniform1i (GLint location, GLint v0);
typedef void        Type_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void        Type_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef GLint       Type_glGetAttribLocation (GLuint program, const GLchar *name);
typedef void        Type_glEnableVertexAttribArray (GLuint index);
typedef void        Type_glGenVertexArrays (GLsizei n, GLuint *arrays);
typedef void        Type_glBindVertexArray (GLuint array);
typedef void        Type_glBindAttribLocation (GLuint program, GLuint index, const GLchar *name);
typedef void        Type_glDebugMessageCallbackARB (GLDEBUGPROCARB callback, const void *userParam);
typedef void        Type_glDisableVertexAttribArray (GLuint index);
typedef void        Type_glUniform3fv (GLint location, GLsizei count, const GLfloat *value);
typedef void        Type_glVertexAttribIPointer (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
typedef void        Type_glUniform4fv (GLint location, GLsizei count, const GLfloat *value);
typedef void        Type_glVertexAttribDivisor (GLuint index, GLuint divisor);
typedef void        Type_glDrawElementsInstanced (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
typedef void        Type_glUniform1f (GLint location, GLfloat v0);
typedef void        Type_glUniform1fv (GLint location, GLsizei count, const GLfloat *value);
typedef void        Type_glGenFramebuffers (GLsizei n, GLuint *framebuffers);
typedef void        Type_glBindFramebuffer (GLenum target, GLuint framebuffer);
typedef void        Type_glFramebufferTexture2D (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void        Type_glTexStorage3D (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
typedef void        Type_glTexSubImage3D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
typedef void        Type_glGenerateMipmap (GLenum target);
typedef void        Type_glBindImageTexture (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
typedef void        Type_glClearTexImage (GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
typedef void        Type_glDrawBuffers (GLsizei n, const GLenum *bufs);
typedef void        Type_glActiveTexture (GLenum texture);
typedef void        Type_glBindRenderbuffer (GLenum target, GLuint renderbuffer);
typedef void        Type_glRenderbufferStorage (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void        Type_glFramebufferRenderbuffer (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void        Type_glGenRenderbuffers (GLsizei n, GLuint *renderbuffers);
typedef void        Type_glBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
typedef void        Type_glBufferStorage (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
typedef void        Type_glBindBufferBase (GLenum target, GLuint index, GLuint buffer);
typedef void        Type_glGetBufferSubData (GLenum target, GLintptr offset, GLsizeiptr size, void *data);
typedef void        Type_glTexBuffer (GLenum target, GLenum internalformat, GLuint buffer);
typedef void        Type_glUniform1ui (GLint location, GLuint v0);
typedef void        Type_glDispatchCompute (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
typedef void        Type_glMemoryBarrier (GLbitfield barriers);
typedef void *      Type_glMapBufferRange (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
typedef GLboolean   Type_glUnmapBuffer (GLenum target);
typedef void        Type_glGetIntegeri_v (GLenum target, GLuint index, GLint *data);
typedef void        Type_glDeleteBuffers (GLsizei n, const GLuint *buffers);
typedef void        Type_glClearNamedBufferData (GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
typedef void        Type_glDeleteFramebuffers (GLsizei n, const GLuint *framebuffers);
typedef void        Type_glDeleteRenderbuffers (GLsizei n, const GLuint *renderbuffers);
typedef void        Type_glGenerateTextureMipmap (GLuint texture);
typedef void        Type_glPatchParameteri(GLenum pname, GLint value);
typedef void        Type_glDeleteShader(GLuint shader);
typedef void        Type_glFramebufferTexture (GLenum target, GLenum attachment, GLuint texture, GLint level);
typedef void        Type_glTexImage3D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
typedef GLenum      Type_glCheckFramebufferStatus (GLenum target);
typedef void        Type_glUniform4f (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void        Type_glUniform2f (GLint location, GLfloat v0, GLfloat v1);
typedef void        Type_glGetUniformiv (GLuint program, GLint location, GLint *params);
typedef void        Type_glGetActiveUniform (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void        Type_glGetActiveAttrib (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
typedef void        Type_glDrawArraysInstanced (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef void        Type_glGetShaderiv (GLuint shader, GLenum pname, GLint *params);
typedef void        Type_glCreateTextures(GLenum target, GLsizei n, GLuint *textures);
typedef void        Type_glTextureParameteri (GLuint texture, GLenum pname, GLint param);
typedef void        Type_glTextureStorage2D (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
typedef void        Type_glTextureSubImage2D (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);


// #Note: Function Pointers
//
#define OPENGL_FUNCTION(Name) Type_##Name *Name
OPENGL_FUNCTION(glCreateShader);
OPENGL_FUNCTION(glShaderSource);
OPENGL_FUNCTION(glCompileShader);
OPENGL_FUNCTION(glCreateProgram);
OPENGL_FUNCTION(glAttachShader);
OPENGL_FUNCTION(glLinkProgram);
OPENGL_FUNCTION(glGetProgramiv);
OPENGL_FUNCTION(glGetShaderInfoLog);
OPENGL_FUNCTION(glValidateProgram);
OPENGL_FUNCTION(glGetProgramInfoLog);
OPENGL_FUNCTION(glGenBuffers);
OPENGL_FUNCTION(glBindBuffer);
OPENGL_FUNCTION(glUniformMatrix4fv);
OPENGL_FUNCTION(glGetUniformLocation);
OPENGL_FUNCTION(glUseProgram);
OPENGL_FUNCTION(glUniform1i);
OPENGL_FUNCTION(glBufferData);
OPENGL_FUNCTION(glVertexAttribPointer);
OPENGL_FUNCTION(glGetAttribLocation);
OPENGL_FUNCTION(glEnableVertexAttribArray);
OPENGL_FUNCTION(glGenVertexArrays);
OPENGL_FUNCTION(glBindVertexArray);
OPENGL_FUNCTION(glBindAttribLocation);
OPENGL_FUNCTION(glDebugMessageCallbackARB);
OPENGL_FUNCTION(glDisableVertexAttribArray);
OPENGL_FUNCTION(glUniform3fv);
OPENGL_FUNCTION(glVertexAttribIPointer);
OPENGL_FUNCTION(glUniform4fv);
OPENGL_FUNCTION(glVertexAttribDivisor);
OPENGL_FUNCTION(glDrawElementsInstanced);
OPENGL_FUNCTION(glUniform1f);
OPENGL_FUNCTION(glUniform1fv);
OPENGL_FUNCTION(glGenFramebuffers);
OPENGL_FUNCTION(glBindFramebuffer);
OPENGL_FUNCTION(glFramebufferTexture2D);
OPENGL_FUNCTION(glTexStorage3D);
OPENGL_FUNCTION(glTexSubImage3D);
OPENGL_FUNCTION(glGenerateMipmap);
OPENGL_FUNCTION(glBindImageTexture);
OPENGL_FUNCTION(glClearTexImage);
OPENGL_FUNCTION(glDrawBuffers);
OPENGL_FUNCTION(glActiveTexture);
OPENGL_FUNCTION(glBindRenderbuffer);
OPENGL_FUNCTION(glRenderbufferStorage);
OPENGL_FUNCTION(glFramebufferRenderbuffer);
OPENGL_FUNCTION(glGenRenderbuffers);
OPENGL_FUNCTION(glBufferSubData);
OPENGL_FUNCTION(glBufferStorage);
OPENGL_FUNCTION(glBindBufferBase);
OPENGL_FUNCTION(glGetBufferSubData);
OPENGL_FUNCTION(glTexBuffer);
OPENGL_FUNCTION(glUniform1ui);
OPENGL_FUNCTION(glDispatchCompute);
OPENGL_FUNCTION(glMemoryBarrier);
OPENGL_FUNCTION(glMapBufferRange);
OPENGL_FUNCTION(glUnmapBuffer);
OPENGL_FUNCTION(glGetIntegeri_v);
OPENGL_FUNCTION(glDeleteBuffers);
OPENGL_FUNCTION(glClearNamedBufferData);
OPENGL_FUNCTION(glDeleteFramebuffers);
OPENGL_FUNCTION(glDeleteRenderbuffers);
OPENGL_FUNCTION(glGenerateTextureMipmap);
OPENGL_FUNCTION(glPatchParameteri);
OPENGL_FUNCTION(glDeleteShader);
OPENGL_FUNCTION(glFramebufferTexture);
OPENGL_FUNCTION(glTexImage3D);
OPENGL_FUNCTION(glCheckFramebufferStatus);
OPENGL_FUNCTION(glUniform4f);
OPENGL_FUNCTION(glUniform2f);
OPENGL_FUNCTION(glGetUniformiv);
OPENGL_FUNCTION(glGetActiveUniform);
OPENGL_FUNCTION(glGetActiveAttrib);
OPENGL_FUNCTION(glDrawArraysInstanced);
OPENGL_FUNCTION(glGetShaderiv);
OPENGL_FUNCTION(glCreateTextures);
OPENGL_FUNCTION(glTextureParameteri);
OPENGL_FUNCTION(glTextureStorage2D);
OPENGL_FUNCTION(glTextureSubImage2D);





// All you have to do is append the desired function! X macro will do the heavy lifting!
#define GL_FUNCTIONS(X) \
    X(PFNGLGETSTRINGIPROC,                      glGetStringi) \
    X(PFNGLTEXTUREPARAMETERFPROC,               glTextureParameterf) \
    X(PFNGLTEXTURESUBIMAGE3DPROC,               glTextureSubImage3D) \
    X(PFNGLTEXTURESTORAGE3DPROC,                glTextureStorage3D) \
    X(PFNGLTEXTUREPARAMETERFVPROC,              glTextureParameterfv) \
    X(PFNGLBINDTEXTUREUNITPROC,                 glBindTextureUnit) \
    X(PFNGLCREATEFRAMEBUFFERSPROC,              glCreateFramebuffers) \
    X(PFNGLNAMEDFRAMEBUFFERTEXTUREPROC,         glNamedFramebufferTexture) \
    X(PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC,     glCheckNamedFramebufferStatus) \
    X(PFNGLCLEARNAMEDFRAMEBUFFERFVPROC,         glClearNamedFramebufferfv) \
    X(PFNGLDEBUGMESSAGECONTROLPROC,             glDebugMessageControl)



#define X(type, name) static type name;
GL_FUNCTIONS(X)
#undef X
