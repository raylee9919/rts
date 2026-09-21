// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GENERATED_MATERIAL_H
#define RTS_GENERATED_MATERIAL_H

#include "basic/core.h"
#include "gfx/gfx.h"
#include "shaders/shared/shared.h"

struct Material_Field_Info {
  String name;
  u64    size;
  u64    offset;
};

struct IMaterial {
  virtual Pair<u32, Material_Field_Info*> get_field_infos() = 0;
  virtual u64 get_gpu_material_size() = 0;
  virtual void write_gpu_material(void *dst) = 0;
};

// ------------------------------------------------------------------------- //

struct Mtl_Doggo : public IMaterial {
  float3 albedo;
  float metallic;
  float roughness;
  Guid albedo_id;
  Guid orm_id;

  static String shader_source;
  static Material_Field_Info field_info[5];

  struct GPU_Material {
    float3 albedo;
    float metallic;
    float roughness;
    uint32_t albedo_id;
    uint32_t orm_id;
  };

  virtual Pair<u32, Material_Field_Info*> get_field_infos() override {
    return { array_count(field_info), field_info };
  }

  virtual u64 get_gpu_material_size() override {
    return sizeof(GPU_Material);
  }

  virtual void write_gpu_material(void *dst) override {
    GPU_Material *m = (GPU_Material *)dst;
    m->albedo = albedo;
    m->metallic = metallic;
    m->roughness = roughness;
    m->albedo_id = gfx_srv_bindless_from_texture(albedo_id);
    m->orm_id = gfx_srv_bindless_from_texture(orm_id);
  }

};

#ifdef GENERATED_MATERIAL_IMPLEMENTATION
String Mtl_Doggo::shader_source = S("doggo.slang");
Material_Field_Info Mtl_Doggo::field_info[5] = {
    { S("albedo"), sizeof(albedo), offset_of(Mtl_Doggo, albedo) },
    { S("metallic"), sizeof(metallic), offset_of(Mtl_Doggo, metallic) },
    { S("roughness"), sizeof(roughness), offset_of(Mtl_Doggo, roughness) },
    { S("albedo_id"), sizeof(albedo_id), offset_of(Mtl_Doggo, albedo_id) },
    { S("orm_id"), sizeof(orm_id), offset_of(Mtl_Doggo, orm_id) },
};
#endif

#endif // RTS_GENERATED_MATERIAL_H