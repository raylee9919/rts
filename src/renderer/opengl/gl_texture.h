// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

struct Opengl;

namespace GL
{
    struct Texture
    {
        u64      id;      // asset id
        GLuint   name;    // obtained with glCreateTextures()
        GLuint64 handle;  // bindless texture handle

        b32      committed;

        Texture* next;
        Texture* prev;
    };

    Texture  *alloc_texture_unique(Opengl* gl, u64 id, Texture_Layout layout, u32 width, u32 height, void* data);
    void     commit_texture(Opengl*gl, u64 id);
    void     decommit_texture(Opengl*gl, u64 id);
    Texture  *get_texture(Opengl* gl, u64 id);
}
