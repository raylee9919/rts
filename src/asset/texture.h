// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

namespace Asset
{
    struct Texture 
    {
        // @Todo: Stable ID?
        // Unstable ID. May differ every time you play a game.
        u64 incremental_id;

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
        Texture_Layout layout;
    };

    // APIs
    //
    void load_texture(System *sys, Texture *tex, void *memory, u64 size);

    void load_texture(System *sys, Texture *tex, Utf8 file_path);

    void import_texture(System *sys, Texture *tex, Utf8 file_path, bool flip);

    void export_texture(System *sys, Texture *tex, Utf8 file_path);

    void store_texture(System *sys, Texture *tex, Utf8 file_path);


    // Internal
    //
    Texture_Layout determine_layout(u32 bytes_per_channel, u32 num_channels);
}
