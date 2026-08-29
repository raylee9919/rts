// Copyright Seong Woo Lee. All Rights Reserved.

struct Opengl;

//internal void gl_alloc_texture(Opengl *gl, Asset::Texture *texture, GLenum wrapping, bool generate_mipmap = false);

internal void gl_alloc_texture2(Opengl *gl, const Render_Command& cmd, GLuint* table);

internal void gl_create_default_colored_texture(Opengl *gl, u32 rgba, void *data, GLuint *out_handle);

internal GLuint gl_create_cubemap_texture(Texture_Layout layout_, int width, int height, void *data, int stride, int offset);
