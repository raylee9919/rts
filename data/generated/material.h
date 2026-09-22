// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GENERATED_MATERIAL_H
#define RTS_GENERATED_MATERIAL_H

#include "basic/core.h"
#include "os/os.h"
#include "gfx/gfx.h"
#include "shaders/shared/shared.h"

#define MAX_MATERIAL_FIELDS 32

enum Material_Field_Info_Type {
  MATERIAL_FIELD_SCALAR,
  MATERIAL_FIELD_ASSET
};

struct Material_Field_Info {
  Material_Field_Info_Type type;
  String                   name;
  u64                      size;
  u64                      offset;
};

// Per-type behaviour. Materials themselves are plain data.
struct Material_Base {
  u64  (*get_gpu_material_size)(void);
  void (*write_gpu_material)(Material_Base *material, void *dst);
};

struct Material_Type_Info {
  Material_Base       base;
  u64                 cpu_size;
  u32                 num_fields;
  Material_Field_Info fields[MAX_MATERIAL_FIELDS];
};

extern Table<Guid, Material_Type_Info, hash_guid> material_type_table;

void init_material_type_table();

// ------------------------------------------------------------------------- //

struct Mtl_Doggo
{
  Material_Base base;

  Guid albedo_id;
  Guid orm_id;

  struct GPU_Material {
    uint32_t albedo_id;
    uint32_t orm_id;
  };
};

u64  Mtl_Doggo__get_gpu_material_size(void);
void Mtl_Doggo__write_gpu_material(Material_Base *material, void *dst);
void Mtl_Doggo__init();

// ------------------------------------------------------------------------- //

struct Mtl_Knight
{
  Material_Base base;

  Guid albedo_id;

  struct GPU_Material {
    uint32_t albedo_id;
  };
};

u64  Mtl_Knight__get_gpu_material_size(void);
void Mtl_Knight__write_gpu_material(Material_Base *material, void *dst);
void Mtl_Knight__init();

#endif // RTS_GENERATED_MATERIAL_H

#if defined(GENERATED_MATERIAL_IMPLEMENTATION) && !defined(RTS_GENERATED_MATERIAL_IMPL)
#define RTS_GENERATED_MATERIAL_IMPL

u64 Mtl_Doggo__get_gpu_material_size(void) {
  return sizeof(Mtl_Doggo::GPU_Material);
}

void Mtl_Doggo__write_gpu_material(Material_Base *material, void *dst) {
  Mtl_Doggo *m = (Mtl_Doggo *)material;
  Mtl_Doggo::GPU_Material *gpu = (Mtl_Doggo::GPU_Material *)dst;
  gpu->albedo_id = gfx_srv_bindless_from_texture(m->albedo_id);
  gpu->orm_id = gfx_srv_bindless_from_texture(m->orm_id);
}

void Mtl_Doggo__init() {
  Material_Type_Info info = {};
  info.base.get_gpu_material_size = Mtl_Doggo__get_gpu_material_size;
  info.base.write_gpu_material    = Mtl_Doggo__write_gpu_material;
  info.cpu_size   = sizeof(Mtl_Doggo);
  info.num_fields = 2;
  info.fields[0] = { MATERIAL_FIELD_ASSET, S("albedo_id"), sizeof(Mtl_Doggo::albedo_id), offset_of(Mtl_Doggo, albedo_id) };
  info.fields[1] = { MATERIAL_FIELD_ASSET, S("orm_id"), sizeof(Mtl_Doggo::orm_id), offset_of(Mtl_Doggo, orm_id) };
  table_add(&material_type_table, guid_from_string(S("shaders/material/doggo.slang")), info);
}

u64 Mtl_Knight__get_gpu_material_size(void) {
  return sizeof(Mtl_Knight::GPU_Material);
}

void Mtl_Knight__write_gpu_material(Material_Base *material, void *dst) {
  Mtl_Knight *m = (Mtl_Knight *)material;
  Mtl_Knight::GPU_Material *gpu = (Mtl_Knight::GPU_Material *)dst;
  gpu->albedo_id = gfx_srv_bindless_from_texture(m->albedo_id);
}

void Mtl_Knight__init() {
  Material_Type_Info info = {};
  info.base.get_gpu_material_size = Mtl_Knight__get_gpu_material_size;
  info.base.write_gpu_material    = Mtl_Knight__write_gpu_material;
  info.cpu_size   = sizeof(Mtl_Knight);
  info.num_fields = 1;
  info.fields[0] = { MATERIAL_FIELD_ASSET, S("albedo_id"), sizeof(Mtl_Knight::albedo_id), offset_of(Mtl_Knight, albedo_id) };
  table_add(&material_type_table, guid_from_string(S("shaders/material/knight.slang")), info);
}

void init_material_type_table() {
  Mtl_Doggo__init();
  Mtl_Knight__init();
}

#endif // GENERATED_MATERIAL_IMPLEMENTATION
