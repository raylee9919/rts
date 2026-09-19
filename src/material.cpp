// Copyright Seong Woo Lee. All Rights Reserved.

#include "./material.h"
#include "os/os.h"
#include "basic/context.h"
#include "basic/log.h"
#include "text_file_handler/text_file_handler.h"
#include "renderer/renderer.h"
#include "asset/asset.h"
#include "shared.h"
#include "third_party/stb/stb_image.h"


static RHI_Format bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit);
static Bitmap bitmap_import(void *loaded_data, u64 size);
static void bitmap_free(Bitmap *bitmap);


void material_load_proc( String filepath, String short_name, void *user_data )
{
    auto parse_string = [](Text_File_Handler *handler, String val) -> Pair<b32, String> {
        if (val.len <= 2) { 
            log_error(S("Expected string at line: %d"), handler->line_number);
            return { false, {} };
        }

        if ( !begins_with(val, S("\"")) ) {
            log_error(S("Expected '\"' at the beginning of the string, at line: %d, but encountered '%S'"), handler->line_number, val[0]);
            return { false, {} };
        }

        if ( !ends_with(val, S("\"")) ) {
            log_error(S("Expected '\"' at the end of the string, at line: %d, but encountered '%S'"), handler->line_number, val[val.len - 1]);
            return { false, {} };
        }

        // Trim "
        String s = val;
        s.len -= 2;
        s.str += 1;

        return { true, s };
    };

    Text_File_Handler handler = {};
    String s = read_entire_file(filepath, tctx.temp);
    handler.start(s);

    Material material = {};
    Guid material_id = guid_from_string(short_name);

    while (1) 
    {
        auto [line, found] = handler.consume_next_line();
        if (!found) break;

        auto [field, rhs] = break_by_spaces(line);
        auto [colon, val] = break_by_spaces(rhs);

        if (colon != S(":")) {
            log_error(S("Expected ':' at line: %d, but encountered '%S'"), handler.line_number, colon);
            return;
        }

        // @Cleanup
        // Manually typing fields names is cubersome. 
        // C/C++ sucks. It's either dirty macro or code gen. 

        // @Cleanup: 
        // Should I do DAG thing or what.
        // Load assets this is depending on:

        if (field == S("albedo_texture")) {
            auto [success, s] = parse_string(&handler, val);
            if (!success) return;

            asset_request(guid_from_string(s));
            material.albedo_texture = guid_from_string(s);
        }
        else if (field == S("orm_texture")) {
            auto [success, s] = parse_string(&handler, val);
            if (!success) return;

            asset_request(guid_from_string(s));
            material.orm_texture = guid_from_string(s);
        }
        else if (field == S("shader")) {
            auto [success, s] = parse_string(&handler, val);
            if (!success) return;

            // @Temporary
            // If I ever decide to have a material instance, instances 
            // share a base material's pipeline, thus, pipeline id must be 
            // discriminated from material id.
            Guid pipeline_id = material_id;
            material.pipeline = pipeline_id;

            // `shader` is the material implementation (IMaterial). It gets linked
            // into the template shader, which owns the entry points.
            // @Temporary: Single template for every material.
            String template_path = tprint(S("%S/shaders/pass/surface.slang"), shared->data_path);
            String material_path = tprint(S("%S/%S"), shared->data_path, s);
            r_pipeline_create(pipeline_id, template_path, material_path, SHADING_MODEL_OPAQUE);
        }
        else {
            log_error(S("Unexpected field name '%S', at line: %d"), field, handler.line_number);
        }
    }

    Material *m = r_material_alloc(material_id);
    *m = material;
}

void image_load_proc( String filepath, String short_name, void *user_data )
{
    String contents = read_entire_file(filepath, tctx.temp);
    Bitmap bitmap   = bitmap_import(contents.str, contents.len);

    Guid id = guid_from_string(short_name);

    RHI_Texture_Desc desc = {};
    {
        desc.type           = RHI_TEXTURE_TYPE_2D;
        desc.format         = bitmap.format;
        desc.usage          = RHI_TEXTURE_USAGE_SAMPLED;
        desc.width          = bitmap.width;
        desc.height         = bitmap.height;
        desc.mip_levels     = 1;
        desc.depth          = 1;
    }
    gfx_texture_create(id, desc);
    gfx_texture_upload(id, desc.format, bitmap.data, bitmap.size, bitmap.width, bitmap.height);

    bitmap_free(&bitmap);
}

static RHI_Format bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit) {
    if (is_hdr) {
        Assert(!"X"); // @Todo: HDR
    } else if (is_16_bit) {
        if (num_channels == 1) return RHI_FORMAT_R16_UNORM;
        if (num_channels == 2) return RHI_FORMAT_RG16_UNORM;
        if (num_channels == 4) return RHI_FORMAT_RGBA16_UNORM;
        else Assert(!"Unsupported 16-bit channel count");
    } else {
        if (num_channels == 1) return RHI_FORMAT_R8_UNORM;
        if (num_channels == 2) return RHI_FORMAT_RG8_UNORM;
        if (num_channels == 4) return RHI_FORMAT_RGBA8_UNORM;
        else Assert(!"Unsupported channel count");
    }
    return RHI_FORMAT_UNKNOWN;
}

static Bitmap bitmap_import(void *loaded_data, u64 size) {
    Bitmap result = {};

    u8 *data = (u8*)loaded_data;
    int sz = (int)size;

    int w, h, num_channels;

    stbi_info_from_memory(data, sz, &w, &h, &num_channels);
    
    int force_channel = num_channels == 3 ? 4 : num_channels;

    b32 is_hdr    = stbi_is_hdr_from_memory(data, sz);
    b32 is_16_bit = stbi_is_16_bit_from_memory(data, sz);

    if (is_hdr) {
        Assert(!"X"); // @Todo: HDR
    }

    void *ptr = NULL;
    if (is_16_bit) {
        ptr = stbi_load_16_from_memory(data, sz, &w, &h, &num_channels, force_channel);
    } else {
        ptr = stbi_load_from_memory(data, sz, &w, &h, &num_channels, force_channel);
    }

    if (ptr) {
        result.format = bitmap_compute_format(force_channel, is_hdr, is_16_bit);
        result.data   = ptr;
        result.size   = w * h * force_channel * (is_16_bit ? 2 : 1);
        result.width  = w;
        result.height = h;
    }

    return result;
}

static void bitmap_free(Bitmap *bitmap) {
    stbi_image_free(bitmap->data);
}
