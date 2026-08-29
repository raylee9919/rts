// Copyright Seong Woo Lee. All Rights Reserved.


#include "third_party/stb/stb_image.h"
#include "third_party/stb/stb_image_write.h"


namespace Asset
{
    struct Texture 
    {
        // @Todo: Stable ID?
        // Unstable ID. May differ every time you play a game.
        u64 incremental_id;

        // Read
        String name;

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

    void load_texture(System *sys, Texture *tex, String file_path);

    void import_texture(System *sys, Texture *tex, String file_path, bool flip);

    void export_texture(System *sys, Texture *tex, String file_path);

    void store_texture(System *sys, Texture *tex, String file_path);


    // Internal
    //
    Texture_Layout determine_layout(u32 bytes_per_channel, u32 num_channels);
}
