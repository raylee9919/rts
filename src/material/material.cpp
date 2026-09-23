// Copyright Seong Woo Lee. All Rights Reserved.

#include "./material.h"
#include "os/os.h"
#include "basic/context.h"
#include "basic/log.h"
#include "text_file_handler/text_file_handler.h"
#include "renderer/renderer.h"
#include "asset/asset.h"
#include "asset/parser.h"
#include "shared.h"
#include "shader_compiler/shader.h"
#include "third_party/stb/stb_image.h"



static RHI_Format bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit);
static Bitmap bitmap_import(void *loaded_data, u64 size);
static void bitmap_free(Bitmap *bitmap);


//
// Material System
//
static Allocator material_system_allocator;
static Pair<u64, u64> material_field_type_size_align[M_FIELD_TYPE_COUNT];
static Table<Guid, M_TypeInfo, hash_guid> material_type_table;
static Table<Guid, M_Entry, hash_guid> material_table;

static u64 get_size(M_FieldType type) {
    return material_field_type_size_align[type].x;
}

static u64 get_align(M_FieldType type) {
    return material_field_type_size_align[type].y;
}

void write_gpu_material(M_TypeInfo *info, void *gpu_ptr, void *material) 
{
    // With each field's type, offset and size, we 
    // can convert and wrtie data to GPU-side buffer.
    auto convert = [](u8 *src, M_FieldType cpu_type, M_FieldType gpu_type) -> u8* {
        u8 *data = src;
        if (cpu_type == gpu_type) return data;
        if (cpu_type == M_FIELD_GUID) {
            R_ASSERT(gpu_type == M_FIELD_TEXTURE);
            data = alloc(4, tctx.temp);
            Guid guid = *(Guid*)src;
            u32 bindless = gfx_srv_bindless_from_texture(guid);
            *(u32*)data = bindless;
        }
        return data;
    };

    for (u32 i = 0; i < info->num_fields; ++i) {
        M_Field field = info->fields[i];
        u8 *dst = (u8*)gpu_ptr  + field.gpu_offset;
        u8 *src = convert((u8*)material + field.cpu_offset, field.cpu_type, field.gpu_type);
        memcpy(dst, src, info->fields[i].gpu_size);
    }
}

static String get_material_shortname(String path)
{
    // @Robustness
    String short_name = path;
    R_ASSERT(begins_with(path, shared->data_path));
    advance(&short_name, shared->data_path.len);
    short_name = trim_left(short_name, S("./\\"));

    // @Robustness: Path normalization
    u8 *dst = alloc(short_name.len + 1, tctx.temp);
    s64 len = 0;
    for (s64 i = 0; i < short_name.len; ++i) {
        u8 c = short_name.str[i];
        if (c == '\\')  c = '/';
        if (c == '/' && len > 0 && dst[len - 1] == '/')  continue;
        dst[len++] = c;
    }
    dst[len] = 0;

    return String{ dst, len };
}

static M_FieldType convert_type(Shader_Field_Type type, b32 is_gpu) {
    switch (type) {
        case SHADER_FIELD_INT8:    return M_FIELD_S8;
        case SHADER_FIELD_INT16:   return M_FIELD_S16;
        case SHADER_FIELD_INT32:   return M_FIELD_S32;
        case SHADER_FIELD_INT64:   return M_FIELD_S64;

        case SHADER_FIELD_UINT8:   return M_FIELD_U8;
        case SHADER_FIELD_UINT16:  return M_FIELD_U16;
        case SHADER_FIELD_UINT32:  return M_FIELD_U32;
        case SHADER_FIELD_UINT64:  return M_FIELD_U64;

        case SHADER_FIELD_FLOAT:   return M_FIELD_F32;
        case SHADER_FIELD_FLOAT2:  return M_FIELD_VEC2;
        case SHADER_FIELD_FLOAT3:  return M_FIELD_VEC3;
        case SHADER_FIELD_FLOAT4:  return M_FIELD_VEC4;

        default: R_ASSERT(0); return M_FIELD_S32;
    }
}

Pair<b32, M_TypeInfo> get_material_type_info(Shader_Compiler *shader_compiler, String filepath)
{
    auto [reflect_ok, mtl] = shader_reflect_material(shader_compiler, filepath);
    if ( !reflect_ok ) {
        log_error(S("Error while reflecting '%S'"), filepath);
        return {false, {}};
    }

    String shortname = get_material_shortname(filepath);

    M_TypeInfo type_info = {};
    type_info.id         = guid_from_string(shortname);
    type_info.num_fields = mtl.num_fields;

    u64 cpu_align = 1;
    u64 gpu_align = 1;

    for (u32 i = 0; i < mtl.num_fields; ++i) {
        Shader_Field field = mtl.fields[i];

        M_Field *f = &type_info.fields[i];
        f->name = field.name; // @Todo: is it `const char *` in read-only memory?

        f->cpu_type   = field.attribute == S("Texture") ? M_FIELD_GUID : convert_type(field.type, false);
        f->cpu_size   = get_size(f->cpu_type);
        f->cpu_offset = (i > 0) ? align_up(type_info.fields[i-1].cpu_offset + type_info.fields[i-1].cpu_size, get_align(f->cpu_type)) : 0;

        f->gpu_type   = field.attribute == S("Texture") ? M_FIELD_TEXTURE : convert_type(field.type, true);
        f->gpu_size   = get_size(f->gpu_type);
        f->gpu_offset = (i > 0) ? align_up(type_info.fields[i-1].gpu_offset + type_info.fields[i-1].gpu_size, get_align(f->gpu_type)) : 0;

        cpu_align = max(cpu_align, get_align(f->cpu_type));
        gpu_align = max(gpu_align, get_align(f->gpu_type));
    }

    if (mtl.num_fields > 0) {
        M_Field *last = &type_info.fields[mtl.num_fields - 1];
        type_info.cpu_size = align_up(last->cpu_offset + last->cpu_size, cpu_align);
        type_info.gpu_size = align_up(last->gpu_offset + last->gpu_size, gpu_align);
    }

    return {true, type_info};
}

b32 material_system_init(String material_shader_dir, 
                         Shader_Compiler *shader_compiler) 
{
    // Assign allocators
    material_system_allocator = {crt_proc, nullptr};
    material_type_table.allocator = material_system_allocator;
    material_table.allocator      = material_system_allocator;


    // Fill in size/alignment table of field types.
    material_field_type_size_align[M_FIELD_S8]  = { sizeof(s8),  align_of(s8)  };
    material_field_type_size_align[M_FIELD_S16] = { sizeof(s16), align_of(s16) };
    material_field_type_size_align[M_FIELD_S32] = { sizeof(s32), align_of(s32) };
    material_field_type_size_align[M_FIELD_S64] = { sizeof(s64), align_of(s64) };

    material_field_type_size_align[M_FIELD_U8]  = { sizeof(u8),  align_of(u8)  };
    material_field_type_size_align[M_FIELD_U16] = { sizeof(u16), align_of(u16) };
    material_field_type_size_align[M_FIELD_U32] = { sizeof(u32), align_of(u32) };
    material_field_type_size_align[M_FIELD_U64] = { sizeof(u64), align_of(u64) };

    material_field_type_size_align[M_FIELD_F32]  = {           4, align_of(f32) };
    material_field_type_size_align[M_FIELD_VEC2] = {           8, align_of(f32) };
    material_field_type_size_align[M_FIELD_VEC3] = {          12, align_of(f32) };
    material_field_type_size_align[M_FIELD_VEC4] = {          16, align_of(f32) };

    material_field_type_size_align[M_FIELD_GUID] = { sizeof(Guid), align_of(Guid) };

    material_field_type_size_align[M_FIELD_TEXTURE] = { sizeof(u32), align_of(u32) };


    // Iterate over file list in the directory, reflect, 
    // get type info and push it to the type table.
    Array<String> list = file_list(material_shader_dir, tctx.temp, true);
    for (int file_idx = 0; file_idx < list.count; ++file_idx)
    {
        String path = list.data[file_idx];
        auto [ext, ext_success] = path_extension(path);
        if ( !ext_success ) {
            log_error(S("Failed to get extension from '%S'."), path);
            return false;
        }

        if (ext != S("h") && ext != S("slang")) {
            log_error(S("Unrecognized extension '%S' from '%S'."), ext, path);
            return false;
        }

        auto [ok, type_info] = get_material_type_info(shader_compiler, path);
        if (!ok) {
            log_error(S("Failed to get material type info from '%S'"), path);
            return false;
        }

        table_add(&material_type_table, type_info.id, type_info);
    }

    return true;
}

void material_system_shutdown()
{
    release(material_system_allocator);
}

// @Todo: Entities can share a material, thus we can't 
// blindly alloc/dealloc material.
M_Entry *alloc_material(Guid id) {
    M_Entry *entry = table_add(&material_table, id, {});
    return entry;
}

// @Todo: This too
void free_material(Guid id) {
    table_remove(&material_table, id);
}

M_Entry *material_from_guid(Guid id) {
    return table_find_pointer(&material_table, id);
}

void upload_material(Guid id) 
{
    M_Entry *entry = table_find_pointer(&material_table, id);
    R_ASSERT(entry);

    M_TypeInfo *type_info = table_find_pointer(&material_type_table, entry->type_id);
    u64 gpu_size = type_info->gpu_size;

    { // @Temporary
        u8 *ptr = gfx->upload_buffer_mapped + gfx->upload_buffer_used;
        write_gpu_material(type_info, ptr, entry->data);

        rhi_command_buffer_begin(&gfx->copy_buffer);
        {
            rhi_cmd_copy_buffer_to_buffer(&gfx->copy_buffer, &renderer->material_buffer.buffer, &gfx->upload_buffer, renderer->material_buffer_used, gfx->upload_buffer_used, gpu_size);
            entry->offset = renderer->material_buffer_used;
            renderer->material_buffer_used += gpu_size;
        }
        rhi_command_buffer_end(&gfx->copy_buffer);
        RHI_Command_Buffer *buffers[] = {&gfx->copy_buffer};
        rhi_submit(gfx->device, 1, buffers);
        rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_TRANSFER, &gfx->upload_semaphore, gfx->upload_semaphore_value++);

        gfx->upload_buffer_used += type_info->gpu_size;
    }
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

    M_TypeInfo type_info = find_result.value;
    u32 num_fields = type_info.num_fields;

    if (type_info.cpu_size > MAX_CPU_MATERIAL_SIZE) {
        log_error(S("Material on CPU-side is bigger than max size: '%d'."), MAX_CPU_MATERIAL_SIZE);
        return;
    }


    /* Material entry to fill in */
    Guid material_asset_id = guid_from_string(short_name);
    Guid alloc_id = material_asset_id;
    M_Entry *material = alloc_material(alloc_id);
    material->type_id = type_id;


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


        // Iterate over field infos and match string
        M_Field *field_info = nullptr;
        for (u32 i = 0; i < num_fields; ++i) {
            if (type_info.fields[i].name == field) {
                field_info = type_info.fields + i;
                break;
            }
        }

        if (!field_info) {
            log_error(S("Couldn't find field: '%S'."), field);
            return;
        }

        // @Cleanup: 
        // Should I do DAG thing or what.
        // Load assets this is depending on:

        switch (field_info->cpu_type) {
            case M_FIELD_GUID: {
                auto [success, s] = parse_string(&handler, val);
                if (!success) {
                    log_error(S("Failed to parse field in '%S' at line '%d'."), filepath, handler.line_number);
                    return;
                }

                Guid child_asset = guid_from_string(s);
                asset_request(child_asset);

                memcpy(material->data + field_info->cpu_offset, &child_asset, field_info->cpu_size);
            } break;

            case M_FIELD_S8:
            case M_FIELD_S16:
            case M_FIELD_S32:
            case M_FIELD_S64:
            case M_FIELD_U8:
            case M_FIELD_U16:
            case M_FIELD_U32:
            case M_FIELD_U64:
            {
                auto [int_val, ok, remainder] = int_from_string(val);
                if (!ok) {
                    log_error(S("Failed to convert int from string in '%S' at line '%d'. String was '%S'."), 
                              filepath, handler.line_number, val);
                    return;
                }
            } break;

            // @Todo: float/vector

            default: {
                R_ASSERT(!"Invalid default value");
            } break;
        }
    }


    // @Temporary
    // If I ever decide to have a material instance, instances 
    // share a base material's pipeline, thus, pipeline id must be 
    // discriminated from material id.
    Guid pipeline_id  = alloc_id;
    material->pipeline = pipeline_id;


    // `shader` is the material implementation (IMaterial). It gets linked
    // into the template shader, which owns the entry points.
    // @Temporary: Single template for every material.
    String surface_shader_path  = tprint(S("%S/shaders/pass/surface.slang"), shared->data_path);
    String material_shader_path = tprint(S("%S/%S"), shared->data_path, shader);
    r_pipeline_create(pipeline_id, surface_shader_path, material_shader_path, SHADING_MODEL_OPAQUE);


    // @Fix: As I stated in the function definition, 
    // entites can share material. Blindly alloc/deallocing 
    // material is wrong.
    upload_material(alloc_id);
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

void texture_load_proc( String filepath, String short_name, void *user_data )
{
    // '.texture' is our own cooked format: four ASCII header lines - bytes per
    // channel, channel count, width, height - then a raw uncompressed pixel blob.
    Temporary_Arena scratch = scratch_begin();
    defer( scratch_end(scratch) );

    String contents = read_entire_file(filepath, tctx.temp);
    if ( !contents.str ) {
        log_error(S("Couldn't read texture '%S'."), filepath);
        return;
    }

    Asset::Parser p = {};
    Asset::init(&p, contents.str, contents.len);

    u32 bytes_per_channel = Asset::parse_u32(&p);
    u32 num_channels      = Asset::parse_u32(&p);
    u32 width             = Asset::parse_u32(&p);
    u32 height            = Asset::parse_u32(&p);

    // Exactly one line ending, deliberately not eat_whitespace(): the first pixel
    // byte is free to hold a value that happens to be ASCII whitespace.
    if (p.cursor < p.end && *p.cursor == '\r')  p.cursor += 1;
    if (p.cursor < p.end && *p.cursor == '\n')  p.cursor += 1;

    if ( bytes_per_channel != 1 ) {
        log_error(S("'%S': only 8-bit channels are supported, got %u bytes per channel."),
                  short_name, bytes_per_channel);
        return;
    }

    u64 num_texels    = (u64)width * (u64)height;
    u64 expected_size = num_texels * num_channels;
    if ( (u64)(p.end - p.cursor) < expected_size ) {
        log_error(S("'%S': pixel data is truncated, expected %llu bytes."), short_name, expected_size);
        return;
    }

    // There is no 24-bit format in D3D12, so widen RGB to RGBA - the same thing
    // 'bitmap_import' does for 3-channel images coming through stb.
    u32 dst_channels = (num_channels == 3) ? 4 : num_channels;

    Bitmap bitmap = {};
    bitmap.format = bitmap_compute_format(dst_channels, false, false);
    bitmap.width  = width;
    bitmap.height = height;
    bitmap.size   = num_texels * dst_channels;

    if ( num_channels == dst_channels ) {
        bitmap.data = p.cursor;
    } else {
        u8 *src = p.cursor;
        u8 *dst = push_array_noz(scratch.arena, u8, bitmap.size);
        bitmap.data = dst;

        for (u64 i = 0; i < num_texels; ++i) {
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = 0xff;
            dst += 4;
            src += 3;
        }
    }

    Guid id = guid_from_string(short_name);

    RHI_Texture_Desc desc = {};
    {
        desc.type       = RHI_TEXTURE_TYPE_2D;
        desc.format     = bitmap.format;
        desc.usage      = RHI_TEXTURE_USAGE_SAMPLED;
        desc.width      = bitmap.width;
        desc.height     = bitmap.height;
        desc.mip_levels = 1;
        desc.depth      = 1;
    }
    gfx_texture_create(id, desc);
    gfx_texture_upload(id, desc.format, bitmap.data, (u32)bitmap.size, bitmap.width, bitmap.height);
}

static RHI_Format bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit) {
    if (is_hdr) {
        R_ASSERT(!"X"); // @Todo: HDR
    } else if (is_16_bit) {
        if (num_channels == 1) return RHI_FORMAT_R16_UNORM;
        if (num_channels == 2) return RHI_FORMAT_RG16_UNORM;
        if (num_channels == 4) return RHI_FORMAT_RGBA16_UNORM;
        else R_ASSERT(!"Unsupported 16-bit channel count");
    } else {
        if (num_channels == 1) return RHI_FORMAT_R8_UNORM;
        if (num_channels == 2) return RHI_FORMAT_RG8_UNORM;
        if (num_channels == 4) return RHI_FORMAT_RGBA8_UNORM;
        else R_ASSERT(!"Unsupported channel count");
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
        R_ASSERT(!"X"); // @Todo: HDR
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
