// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

namespace Asset
{
    struct Texture 
    {
        // Read
        Utf8 name;

        u32 bytes_per_channel;
        u32 num_channels;
        u32 width;
        u32 height;
        u8* data;

        // Computed
        u32 pitch;
        u32 size;

        // @Temporary
        u32 handle;
    };

    // Loads data in engine's texture format from memory.
    void load_texture(Texture *tex, void *memory, u64 size);

    // Loads data in engine's texture format from file.
    void load_texture(Texture *tex, Utf8 file_path);

    // Loads file with stb_image and convert to engine's format.
    void import_texture(Texture *tex, Utf8 file_path, bool flip);

    void export_texture(Texture *tex, Utf8 file_path);

    // Stores texture that is in engine's format.
    void store_texture(Texture *tex, Utf8 file_path);
}
