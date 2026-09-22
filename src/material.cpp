// Copyright Seong Woo Lee. All Rights Reserved.

#define GENERATED_MATERIAL_IMPLEMENTATION
#include "./material.h"
#include "os/os.h"
#include "basic/context.h"
#include "basic/string_builder.h"
#include "basic/log.h"
#include "text_file_handler/text_file_handler.h"
#include "renderer/renderer.h"
#include "asset/asset.h"
#include "shared.h"
#include "shader_compiler/shader.h"
#include "third_party/stb/stb_image.h"



static RHI_Format bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit);
static Bitmap bitmap_import(void *loaded_data, u64 size);
static void bitmap_free(Bitmap *bitmap);


Table<Guid, Material_Type_Info, hash_guid> material_type_table;
Table<Guid, Material_Entry, hash_guid> material_table;


void material_type_system_init(Allocator allocator)
{
    material_type_table.allocator = allocator;
    init_material_type_table();

    material_table.allocator = allocator;
}

// @Todo: Entities can share a material, thus we can't 
// blindly alloc/dealloc material.
void material_alloc(Guid guid, Material_Entry *in_entry) 
{
    Material_Entry *entry = table_add(&material_table, guid, *in_entry);

    { // @Temporary
        u8 *ptr = (u8*)rhi_buffer_map(&gfx->upload_buffer);

        Material_Base *base = (Material_Base*)in_entry->data;

        u64 sz = base->get_gpu_material_size();
        base->write_gpu_material(base, ptr);

        rhi_buffer_unmap(&gfx->upload_buffer);

        rhi_command_buffer_begin(&gfx->copy_buffer);
        {
            rhi_cmd_copy_buffer_to_buffer(&gfx->copy_buffer, &renderer->material_buffer.buffer, &gfx->upload_buffer, renderer->material_buffer_used, 0, sz);
            entry->offset = renderer->material_buffer_used;
            renderer->material_buffer_used += sz;
        }
        rhi_command_buffer_end(&gfx->copy_buffer);
        RHI_Command_Buffer *buffers[] = {&gfx->copy_buffer};
        rhi_submit(gfx->device, 1, buffers);
        rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_TRANSFER, &gfx->upload_semaphore, gfx->upload_semaphore_value);

        rhi_semaphore_wait(&gfx->upload_semaphore, gfx->upload_semaphore_value, -1);
        gfx->upload_semaphore_value += 1;
    }
}

b32 material_dealloc(Guid guid) 
{
    // @Todo
    Material_Entry *entry = table_find_pointer(&material_table, guid);
    if ( !entry ) {
        log_error(S("Couldn't find material '%llu-%llu' in the table."), guid._64[1], guid._64[0]);
        return false;
    }
    return true;
}

Material_Entry *material_from_guid(Guid guid) 
{
    Material_Entry *result = table_find_pointer(&material_table, guid);
    Assert(result);
    return result;
}

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
    handler.start(read_entire_file(filepath, tctx.temp));

    Guid material_id = guid_from_string(short_name);

    String shader = {};
    Guid type_id = NULL_GUID;

    {
        auto [line, found] = handler.consume_next_line();
        if ( !found ) {
            log_error(S("Nothing to parse in '%S'."), filepath);
            return;
        }

        if (line[0] != 'C') {
            log_error(S("Expected 'C', but encounterd %S."), break_by_spaces(line).x);
            return;
        }

        line = advance(line, 1);
        line = trim_left(line);

        auto [success, _shader] = parse_string(&handler, line);
        if ( !success ) return;

        type_id = guid_from_string(_shader);

        shader = _shader;
    }


    /* Find material's type info */
    auto find_result = table_find(&material_type_table, type_id);
    if ( !find_result.found ) {
        log_error(S("Couldn't find material: '%S' info."), shader);
        return;
    }

    Material_Type_Info type_info = find_result.value;
    u32 num_fields = type_info.num_fields;
    Material_Field_Info *field_info = type_info.fields;

    if ( type_info.cpu_size > MAX_CPU_MATERIAL_SIZE ) {
        log_error(S("Material '%S' needs %llu bytes, but MAX_CPU_MATERIAL_SIZE is %d."),
                  shader, type_info.cpu_size, MAX_CPU_MATERIAL_SIZE);
        return;
    }


    /* Material entry to fill in */
    Material_Entry material = {};
    material.type_id = type_id;
    memcpy(material.data, &type_info.base, sizeof(type_info.base)); // fill in base material


    /* Parse */
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

        // @Cleanup: 
        // Should I do DAG thing or what.
        // Load assets this is depending on:

        auto find_field = [](String name, Material_Field_Info *infos, u32 count) -> Material_Field_Info* {
            for (u32 i = 0; i < count; ++i) {
                if (infos[i].name == name) return infos + i;
            }
            return nullptr;
        };

        Material_Field_Info *info = find_field(field, field_info, num_fields);
        if ( !info ) {
            log_error(S("Couldn't find field: '%S'"), field);
            return;
        }

        if ( info->type == MATERIAL_FIELD_SCALAR ) {
            // @Todo: Parse scalar
        } else if ( info->type == MATERIAL_FIELD_ASSET ) {
            auto [success, s] = parse_string(&handler, val);
            if (!success) return;

            Guid child_asset = guid_from_string(s);
            asset_request(child_asset);
            memcpy( material.data + info->offset, &child_asset, info->size );
        } else {
            log_error(S("Unrecognized field info type."));
            return;
        }
    }


    // @Temporary
    // If I ever decide to have a material instance, instances 
    // share a base material's pipeline, thus, pipeline id must be 
    // discriminated from material id.
    Guid pipeline_id = material_id;
    material.pipeline = pipeline_id;

    // `shader` is the material implementation (IMaterial). It gets linked
    // into the template shader, which owns the entry points.
    // @Temporary: Single template for every material.
    String surface_shader_path  = tprint(S("%S/shaders/pass/surface.slang"), shared->data_path);
    String material_shader_path = tprint(S("%S/%S"), shared->data_path, shader);
    r_pipeline_create(pipeline_id, surface_shader_path, material_shader_path, SHADING_MODEL_OPAQUE);


    // @Fix: As I stated in the function definition, 
    // entites can share material. Blindly alloc/deallocing 
    // material is wrong.
    material_alloc(material_id, &material);
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
