// Copyright Seong Woo Lee. All Rights Reserved.

internal void gl_alloc_texture(Opengl *gl, Bitmap *bitmap, GLenum wrapping, bool generate_mipmap)
{
    glCreateTextures(GL_TEXTURE_2D, 1, &bitmap->handle);
    GLuint tex = bitmap->handle;

    u32 w = bitmap->width;
    u32 h = bitmap->height;
    void *data = bitmap->memory;

    GLsizei levels = generate_mipmap ? (GLsizei)floor(log2(max(w, h))) + 1 : 1;

    if (bitmap->bits_per_channel == 8) {
        switch (bitmap->channel_count) {
            case 1: 
            glTextureStorage2D(tex, levels, GL_R8, w, h);
            glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, data);
            break;

            case 2: 
            glTextureStorage2D(tex, levels, GL_RG8, w, h);
            glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RG, GL_UNSIGNED_BYTE, data);
            break;

            case 3: 
            glTextureStorage2D(tex, levels, GL_RGB8, w, h);
            glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;

            case 4: 
            glTextureStorage2D(tex, levels, GL_RGBA8, w, h);
            glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;

            INVALID_DEFAULT_CASE;
        }
    } else if (bitmap->bits_per_channel == 16) {
        INVALID_CODE_PATH;
    } else {
        INVALID_CODE_PATH;
    }

    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, wrapping);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, wrapping);

    if (generate_mipmap) {
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateTextureMipmap(tex);
        //glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
    } else {
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
}

internal void gl_alloc_texture2(Opengl* gl, const Render_Command& cmd, GLuint* table)
{
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);

    Render_Texture texture = cmd.texture;

    Render_Id id             = texture.id;
    Texture_Layout type      = texture.layout;
    GLsizei width            = (GLsizei)texture.width;
    GLsizei height           = (GLsizei)texture.height;
    const void *data         = (const void *)texture.data;

    table[id.e[0]] = tex;

    bool gen_mipmap = (cmd.flags & RENDER_COMMAND_FLAG_TEXTURE_MIPMAP);
    GLsizei levels = gen_mipmap ? (GLsizei)floor(log2(max(width, height))) + 1 : 1;

    switch (type) {
        case TEXTURE_LAYOUT_R8G8B8A8: {
            glTextureStorage2D(tex, levels, GL_RGBA8, width, height);
            glTextureSubImage2D(tex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        } break;

        INVALID_DEFAULT_CASE;
    }

    // Wrapping
    if (cmd.flags & RENDER_COMMAND_FLAG_TEXTURE_WRAP) {
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }


    // Filtering
    GLenum filter = GL_NEAREST;
    GLenum mipmap_filter = GL_NEAREST_MIPMAP_LINEAR;
    if (cmd.flags & RENDER_COMMAND_FLAG_TEXTURE_FILTER_LINEAR) {
        filter = GL_LINEAR;
        mipmap_filter = GL_LINEAR_MIPMAP_LINEAR;
    }
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, filter);


    // Mipmap filtering
    if (gen_mipmap) {
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, mipmap_filter);
        glGenerateTextureMipmap(tex);
    } else {
        glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, filter);
    }
}

internal void gl_create_default_colored_texture(Opengl *gl, u32 rgba, void *data, GLuint *out_handle)
{
    GLsizei w = 4, h = 4;
    int bpp = 4;
    int pitch = w*bpp;

    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            u32 *x = (u32 *)((u8 *)data + r*pitch + c*bpp);
            *x = rgba;
        }
    }

    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    *out_handle = tex;

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTextureStorage2D(tex, 1, GL_RGBA8, w, h);
    glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

internal GLuint gl_create_cubemap_texture(Texture_Layout layout_, int width, int height, void *data, int stride, int offset)
{
    GLuint tex; 
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &tex);

    GLenum layout = {}, format = {};

    switch (layout_) {
        case TEXTURE_LAYOUT_R8G8B8A8: {
            layout = GL_RGBA8;
            format = GL_RGBA;
        } break;

        case TEXTURE_LAYOUT_R8G8B8: {
            layout = GL_RGB8;
            format = GL_RGB;
        } break;

        INVALID_DEFAULT_CASE;
    }

    // Filtering and wrapping.
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 

    glTextureStorage2D(tex, 1, layout, width, height);

    for (int i = 0; i < 6; ++i) {
        // @Todo: This is a mess. Bitmap has a pointer inside it.
        //        That's why I have to dereference it.
        u8 *ptr = *((u8**)((u8*)data + stride * i + offset));
        glTextureSubImage3D(tex, 0, 0, 0, i, width, height, 1, format, GL_UNSIGNED_BYTE, ptr);
    }

    return tex;
}
