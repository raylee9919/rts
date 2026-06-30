// Copyright Seong Woo Lee. All Rights Reserved.

#include "third_party/stb/stb_image.h"
#include "third_party/stb/stb_image_write.h"

namespace Asset
{
    void load_texture(System *sys, Texture *tex, void *memory, u64 size)
    {
        assert(tex);

        Asset::Parser p = {};
        init(&p, memory, size);

        tex->incremental_id    = sys->next_incremental_id++;
        tex->bytes_per_channel = parse_u32(&p);
        tex->num_channels      = parse_u32(&p);
        tex->width             = parse_u32(&p);
        tex->height            = parse_u32(&p);
        tex->pitch             = tex->width * tex->num_channels * tex->bytes_per_channel;
        tex->size              = tex->pitch * tex->height;
        tex->layout            = determine_layout(tex->bytes_per_channel, tex->num_channels);


        eat_whitespace(&p);
        // @Todo: Alloc data properly.
        tex->data = new u8[tex->size];
        memory_copy(tex->data, p.cursor, tex->size);
        p.cursor += tex->size;

        assert(is_eof(&p));
    }

    void load_texture(System *sys, Texture *tex, String file_path)
    {
        assert(tex);

        Temporary_Arena scratch = scratch_begin();
        defer(scratch_end(scratch));

        String contents = read_entire_file(scratch.arena, file_path);

        load_texture(sys, tex, contents.str, contents.len);
    }

    void import_texture(System *sys, Texture *tex, String file_path, bool flip)
    {
        assert(tex);

        Temporary_Arena scratch = scratch_begin();
        defer(scratch_end(scratch));

        String contents = read_entire_file(scratch.arena, file_path);

        stbi_set_flip_vertically_on_load(flip);
        int x, y, num_channels;
        u8 *data = stbi_load_from_memory(contents.str, (int)contents.len, &x, &y, &num_channels, 0);

        tex->incremental_id    = sys->next_incremental_id++;
        tex->bytes_per_channel = 1; // @Todo: It mustn't be hard-coded.
        tex->num_channels      = num_channels;
        tex->width             = x;
        tex->height            = y;
        tex->pitch             = tex->width * tex->num_channels * tex->bytes_per_channel;
        tex->size              = tex->pitch * tex->height;
        tex->layout            = determine_layout(tex->bytes_per_channel, tex->num_channels);


        // @Todo: Alloc data properly.
        tex->data = new u8[tex->size];
        memory_copy(tex->data, data, tex->size);

        stbi_image_free(data);
    }

    void export_texture(System *sys, Texture *tex, String file_path)
    {
        int ok = stbi_write_png((const char *)file_path.str, tex->width, tex->height, tex->num_channels, tex->data, tex->pitch);
        assert(ok);
    }

    void store_texture(System *sys, Texture *tex, String file_path)
    {
        FILE *f = fopen((const char *)file_path.str, "wb");
        if (f) {
            fprintf(f, "%u\n", tex->bytes_per_channel);
            fprintf(f, "%u\n", tex->num_channels);
            fprintf(f, "%u\n", tex->width);
            fprintf(f, "%u\n", tex->height);
            fwrite(tex->data, tex->size, 1, f);

            fclose(f);
        }
    }


    Texture_Layout determine_layout(u32 bytes_per_channel, u32 num_channels)
    {
        Texture_Layout layout = TEXTURE_LAYOUT_INVALID;

        if (bytes_per_channel == 1) {
            if (num_channels == 4) {
                layout = TEXTURE_LAYOUT_RGBA8;
            } else if (num_channels == 3) {
                layout = TEXTURE_LAYOUT_RGB8;
            } else if (num_channels == 1) {
                layout = TEXTURE_LAYOUT_R8;
            } else { 
                assert(0);
            }
        } else {
            assert(0);
        }

        return layout;
    }
}
