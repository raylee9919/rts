// Copyright Seong Woo Lee. All Rights Reserved.

#include "./material.h"
#include "os/os.h"
#include "basic/context.h"
#include "basic/log.h"
#include "renderer/renderer.h"
#include "asset/lexer.h"
#include "shared.h"
#include "shader_compiler/shader.h"

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

#define STBIW_ASSERT(x)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"



static RHI_Format bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit);
static Bitmap bitmap_import(void *loaded_data, u64 size);
static void bitmap_free(Bitmap *bitmap);


//
// Material System
//
static Allocator material_system_allocator;
static Pair<u64, u64> material_field_type_size_align[MTL_FIELD_TYPE_COUNT];
static Table<Guid, M_TypeInfo, hash_guid> material_type_table;
static Table<Guid, M_Entry, hash_guid> material_table;
static Guid default_material_id;

static u64 get_size(M_FieldType type) {
  return material_field_type_size_align[type].x;
}

static u64 get_align(M_FieldType type) {
  return material_field_type_size_align[type].y;
}

void 
write_gpu_material(M_TypeInfo *info, void *gpu_ptr, void *material) 
{
  // With each field's type, offset and size, we 
  // can convert and wrtie data to GPU-side buffer.
  auto convert = [](u8 *src, M_FieldType cpu_type, M_FieldType gpu_type) -> u8* {
    u8 *data = src;
    if (cpu_type == gpu_type) return data;
    if (cpu_type == MTL_FIELD_GUID) {
      R_ASSERT(gpu_type == MTL_FIELD_TEXTURE);
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

static String 
get_material_shortname(String path)
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

static M_FieldType 
convert_type(Shader_Field_Type type, b32 is_gpu) 
{
  switch (type) {
    case SHADER_FIELD_INT8:    return MTL_FIELD_S8;
    case SHADER_FIELD_INT16:   return MTL_FIELD_S16;
    case SHADER_FIELD_INT32:   return MTL_FIELD_S32;
    case SHADER_FIELD_INT64:   return MTL_FIELD_S64;

    case SHADER_FIELD_UINT8:   return MTL_FIELD_U8;
    case SHADER_FIELD_UINT16:  return MTL_FIELD_U16;
    case SHADER_FIELD_UINT32:  return MTL_FIELD_U32;
    case SHADER_FIELD_UINT64:  return MTL_FIELD_U64;

    case SHADER_FIELD_FLOAT:   return MTL_FIELD_F32;
    case SHADER_FIELD_FLOAT2:  return MTL_FIELD_VEC2;
    case SHADER_FIELD_FLOAT3:  return MTL_FIELD_VEC3;
    case SHADER_FIELD_FLOAT4:  return MTL_FIELD_VEC4;

    default: R_ASSERT(0); return MTL_FIELD_S32;
  }
}

Pair<b32, M_TypeInfo> 
get_material_type_info(Shader_Compiler *shader_compiler, 
                       String           filepath)
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

    f->cpu_type   = field.attribute == S("Texture") ? MTL_FIELD_GUID : convert_type(field.type, false);
    f->cpu_size   = get_size(f->cpu_type);
    f->cpu_offset = (i > 0) ? align_up(type_info.fields[i-1].cpu_offset + type_info.fields[i-1].cpu_size, get_align(f->cpu_type)) : 0;

    f->gpu_type   = field.attribute == S("Texture") ? MTL_FIELD_TEXTURE : convert_type(field.type, true);
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

b32 
material_system_init(String material_shader_dir, 
                     Shader_Compiler *shader_compiler) 
{
  // Assign allocators
  material_system_allocator     = {crt_proc, nullptr};
  material_type_table.allocator = material_system_allocator;
  material_table.allocator      = material_system_allocator;


  // Fill in size/alignment table for each field type.
  material_field_type_size_align[MTL_FIELD_S8]  = { sizeof(s8),  align_of(s8)  };
  material_field_type_size_align[MTL_FIELD_S16] = { sizeof(s16), align_of(s16) };
  material_field_type_size_align[MTL_FIELD_S32] = { sizeof(s32), align_of(s32) };
  material_field_type_size_align[MTL_FIELD_S64] = { sizeof(s64), align_of(s64) };

  material_field_type_size_align[MTL_FIELD_U8]  = { sizeof(u8),  align_of(u8)  };
  material_field_type_size_align[MTL_FIELD_U16] = { sizeof(u16), align_of(u16) };
  material_field_type_size_align[MTL_FIELD_U32] = { sizeof(u32), align_of(u32) };
  material_field_type_size_align[MTL_FIELD_U64] = { sizeof(u64), align_of(u64) };

  material_field_type_size_align[MTL_FIELD_F32]  = {           4, align_of(f32) };
  material_field_type_size_align[MTL_FIELD_VEC2] = {           8, align_of(f32) };
  material_field_type_size_align[MTL_FIELD_VEC3] = {          12, align_of(f32) };
  material_field_type_size_align[MTL_FIELD_VEC4] = {          16, align_of(f32) };

  material_field_type_size_align[MTL_FIELD_GUID] = { sizeof(Guid), align_of(Guid) };

  material_field_type_size_align[MTL_FIELD_TEXTURE] = { sizeof(u32), align_of(u32) };


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


  // Allocate fallback material if user couldn't find 
  // material from given GUID. For exmaple, there may 
  // be a mesh with no material assigned.
  default_material_id = guid_from_string(S("material/default.material"));
  asset_request(default_material_id, NULL_GUID);

  return true;
}

void 
material_system_shutdown()
{
  destroy(material_system_allocator);
}

// @Todo: Entities can share a material, thus we can't 
// blindly alloc/dealloc material.
M_Entry *
alloc_material(Guid id) 
{
  M_Entry *entry = table_add(&material_table, id, {});
  return entry;
}

// @Todo: This as well.
void 
free_material(Guid id) 
{
  table_remove(&material_table, id);
}

M_Entry *
material_from_guid(Guid id) 
{
  M_Entry *entry = table_find_pointer(&material_table, id);
  if (entry)
  {
    return entry;
  }
  else
  {
    return table_find_pointer(&material_table, default_material_id);
  }
}

// @Todo: If material is *static*, we just need one material allocated.
// If it is *dynamic*, we need it per owner.
void 
upload_material(Guid id) 
{
  M_Entry *entry = table_find_pointer(&material_table, id);
  R_ASSERT(entry);

  M_TypeInfo *type_info = table_find_pointer(&material_type_table, entry->type_id);
  u64 gpu_size = type_info->gpu_size;

  { // @Temporary
    u64 upload_offset = gfx_upload_reserve(gpu_size, 16);
    u8 *ptr = gfx->upload_buffer_mapped + upload_offset;
    write_gpu_material(type_info, ptr, entry->data);

    rhi_command_buffer_begin(&gfx->copy_buffer);
    {
      rhi_cmd_copy_buffer_to_buffer(&gfx->copy_buffer, &renderer->material_buffer.buffer, &gfx->upload_buffer, renderer->material_buffer_used, upload_offset, gpu_size);
      entry->offset = renderer->material_buffer_used;
      renderer->material_buffer_used += gpu_size;
    }
    rhi_command_buffer_end(&gfx->copy_buffer);
    RHI_Command_Buffer *buffers[] = {&gfx->copy_buffer};
    rhi_submit(gfx->device, 1, buffers);
    rhi_semaphore_signal(gfx->device, RHI_COMMAND_TYPE_TRANSFER, &gfx->upload_semaphore, gfx->upload_semaphore_value++);
  }
}

ASSET_LOAD_PROC(material_load_proc)
{
  Asset_Lexer lexer = {};
  asset_tokenize(&lexer, id, tctx.temp);


  // Get version
  s64 version = eat_unsigned_int(&lexer);
  R_ASSERT(version == 1);

  // Get base shader. Shader also determines type of the material
  Guid type_id  = eat_reference(&lexer);
  String shader = asset_name_from_guid(type_id);

  // Get material type info from base shader string
  M_TypeInfo *type_info = table_find_pointer(&material_type_table, type_id);
  R_ASSERT(type_info);

  if (type_info->cpu_size > MAX_CPU_MATERIAL_SIZE) 
  {
    log_error(S("Material's CPU-side size is bigger than the max: '%d'."), MAX_CPU_MATERIAL_SIZE);
    return;
  }

  /* Material entry to fill in */
  M_Entry *material = alloc_material(id);
  material->type_id = type_id;

  auto parse_key_val = [&](Asset_Lexer *lexer) {
    Asset_Token *token = peek_token(lexer);

    String field = token->string_value;
    eat_token(lexer);

    R_ASSERT(peek_token(lexer)->type == ':');
    eat_token(lexer);

    // Iterate over field infos and match string
    M_Field *field_info = nullptr;
    {
      for (u32 i = 0; i < type_info->num_fields; ++i) 
      {
        if (type_info->fields[i].name == field) 
        {
          field_info = type_info->fields + i;
          break;
        }
      }

      if (!field_info) 
      {
        log_error(S("Couldn't find field: '%S'."), field);
        return;
      }
    }

    switch (field_info->cpu_type) 
    {
      case MTL_FIELD_GUID: 
      {
        Guid ref = eat_reference(lexer);
        asset_request(ref, referrer);
        memcpy(material->data + field_info->cpu_offset, &ref, field_info->cpu_size);
      }break;

      case MTL_FIELD_S8:
      case MTL_FIELD_S16:
      case MTL_FIELD_S32:
      case MTL_FIELD_S64:
      {
        s64 value = eat_signed_int(lexer);
        memcpy(material->data + field_info->cpu_offset, &value, field_info->cpu_size);
      }break;

      case MTL_FIELD_U8:
      case MTL_FIELD_U16:
      case MTL_FIELD_U32:
      case MTL_FIELD_U64:
      {
        u64 value = eat_unsigned_int(lexer);
        memcpy(material->data + field_info->cpu_offset, &value, field_info->cpu_size);
      }break;

      // @Todo: float/vector

      default: 
      {
        R_ASSERT(!"Invalid default value");
      }break;
    }
  };

  while (1)
  {
    Asset_Token *t = peek_token(&lexer);
    if (t->type == ASSET_TOKEN_END_OF_INPUT)
      break;

    parse_key_val(&lexer);
  }

  material->pipeline = id;


  // @Temporary: Single template for every material.
  String surface_shader_path  = S("shaders/pass/surface.slang");
  r_pipeline_create(id, surface_shader_path, shader, SHADING_MODEL_OPAQUE);


  // @Todo: Handle static/dynamic material variation.
  upload_material(id);
}

ASSET_LOAD_PROC(image_load_proc)
{
  String path = asset_name_from_guid(id);
  String contents = read_entire_file(path, tctx.temp);
  Bitmap bitmap   = bitmap_import(contents.str, contents.len);

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

static RHI_Format 
bitmap_compute_format(int num_channels, b32 is_hdr, b32 is_16_bit) 
{
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

static Bitmap 
bitmap_import(void *loaded_data, u64 size) 
{
  stbi_set_flip_vertically_on_load(1);
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

static void 
bitmap_free(Bitmap *bitmap) 
{
  stbi_image_free(bitmap->data);
}
