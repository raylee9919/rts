// Copyright Seong Woo Lee. All Rights Reserved.

namespace GL
{
    Texture* alloc_texture_unique(Opengl* gl, u64 id, Texture_Layout layout, u32 width, u32 height, void* data)
    {
        list_for(gl->first_texture, it) {
            if (it->id == id) {
                return it;
            }
        }

        Texture* tex = new Texture; // @Temporary
        zero_struct(tex);

        GLuint name = GL_NONE;

        GLsizei levels = (GLsizei)floor(log2(max(width, height))) + 1;

        switch(layout) 
        {
            case TEXTURE_LAYOUT_RGBA8: {
                glCreateTextures(GL_TEXTURE_2D, 1, &name);

                glTextureStorage2D(name, levels, GL_RGBA8, width, height);
                glTextureSubImage2D(name, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

                glTextureParameteri(name, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(name, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTextureParameteri(name, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(name, GL_TEXTURE_WRAP_T, GL_REPEAT);

                glGenerateTextureMipmap(name);
            } break;

            case TEXTURE_LAYOUT_RGB8: {
                glCreateTextures(GL_TEXTURE_2D, 1, &name);

                glTextureStorage2D(name, levels, GL_RGB8, width, height);
                glTextureSubImage2D(name, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);

                glTextureParameteri(name, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(name, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTextureParameteri(name, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(name, GL_TEXTURE_WRAP_T, GL_REPEAT);

                glGenerateTextureMipmap(name);
            } break;

            case TEXTURE_LAYOUT_R8: {
                glCreateTextures(GL_TEXTURE_2D, 1, &name);

                glTextureStorage2D(name, levels, GL_R8, width, height);
                glTextureSubImage2D(name, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, data);

                glTextureParameteri(name, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTextureParameteri(name, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTextureParameteri(name, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTextureParameteri(name, GL_TEXTURE_WRAP_T, GL_REPEAT);

                glGenerateTextureMipmap(name);
            } break;

            INVALID_DEFAULT_CASE;
        }

        GLuint64 handle = glGetTextureHandleARB(name);

        tex->id     = id;
        tex->name   = name;
        tex->handle = handle;

        dll_push_back(gl->first_texture, gl->last_texture, tex);

        return tex;
    }

    void commit_texture(Opengl* gl, u64 id)
    {
        Texture* tex = get_texture(gl, id);
        assert(tex && "Texture isn't allocated.");

        if (!tex->committed) {
            glMakeTextureHandleResidentARB(tex->handle);
            tex->committed = true;
        }
    }

    void decommit_texture(Opengl* gl, u64 id)
    {
        Texture* tex = get_texture(gl, id);
        assert(tex && "Texture isn't allocated.");


        if (tex->committed) {
            glMakeTextureHandleNonResidentARB(tex->handle);
            tex->committed = false;
        }
    }

    Texture* get_texture(Opengl* gl, u64 id)
    {
        list_for(gl->first_texture, it) {
            if (it->id == id) {
                return it;
            }
        }

        return NULL;
    }
}
