// Copyright Seong Woo Lee. All Rights Reserved.

#include "basic/core.h"
#include "basic/context.h"
#include "basic/log.h"
#include "basic/string.h"
#include "basic/string_builder.h"
#include "os/os.h"
#include "shader_compiler/shader.h"
#include "material/codegen.h"
#include "shared.h"

#define MAX_MATERIAL_FIELDS 32

static String codegen_shortname(String path)
{
    String short_name = path;
    Assert(begins_with(path, shared->data_path));
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

void material_codegen(Shader_Compiler *shader_compiler,
                      String material_shader_dir,
                      String output_dir)
{
    String_Builder sb = {};
    init(&sb, tctx.temp);

    String_Builder impl = {};
    init(&impl, tctx.temp);

    append(&sb, S("// Copyright Seong Woo Lee. All Rights Reserved."));

    append(&sb, S(R"GEN(

#ifndef RTS_GENERATED_MATERIAL_H
#define RTS_GENERATED_MATERIAL_H

#include "basic/core.h"
#include "os/os.h"
#include "gfx/gfx.h"
#include "shaders/shared/shared.h"

)GEN"));

    append(&sb, tprint(S("#define MAX_MATERIAL_FIELDS %d"), MAX_MATERIAL_FIELDS));

    append(&sb, S(R"GEN(

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

)GEN"));


    // Get file list
    Array<String> fl = file_list(material_shader_dir, tctx.temp, true);
    Array<String> names = {};
    names.allocator = tctx.temp;

    for (int file_idx = 0; file_idx < fl.count; ++file_idx)
    {
        String path = fl.data[file_idx];
        auto [ext, ext_success] = path_extension(path);
        if ( ext_success ) 
        {
            if (ext == S("h") || ext == S("slang")) { // @Robustness
                auto [success, mtl] = shader_reflect_material(shader_compiler, path);
                if ( !success ) {
                    log_error(S("Error while generating code for material system."));
                    return;
                }

                array_add(&names, mtl.name);

                // @Todo: Create directory if not exist

                String asset_path = codegen_shortname(path);

                if (mtl.num_fields > MAX_MATERIAL_FIELDS) {
                    log_error(S("Material '%S' has %d fields, but MAX_MATERIAL_FIELDS is %d."),
                              mtl.name, mtl.num_fields, MAX_MATERIAL_FIELDS);
                    return;
                }

                append(&sb, S("// ------------------------------------------------------------------------- //\n\n"));

                /* CPU-side struct. Plain data: behaviour lives in Material_Type_Info. */
                append(&sb, tprint(S("struct %S\n"), mtl.name));
                append(&sb, tprint(S("{\n"), mtl.name));
                append(&sb, S("  Material_Base base;\n"));
                append(&sb, S("\n"));

                /* Fields */
                for (u32 i = 0; i < mtl.num_fields; ++i) {
                    Shader_Field field = mtl.fields[i];
                    String type_s = string_from_shader_field_type(field.type);

                    if (field.attribute == S("Texture")) {
                        append(&sb, tprint(S("  Guid %S;\n"), field.name));
                    } else {
                        append(&sb, tprint(S("  %S %S;\n"), type_s, field.name));
                    }
                }

                append(&sb, S("\n"));

                /* GPU-side struct. This is inside CPU-side one. */
                append(&sb, S("  struct GPU_Material {\n"));
                for (u32 i = 0; i < mtl.num_fields; ++i) {
                    Shader_Field field = mtl.fields[i];
                    String type_s = string_from_shader_field_type(field.type);
                    append(&sb, tprint(S("    %S %S;\n"), type_s, field.name));
                }
                append(&sb, S("  };\n"));

                append(&sb, S("};\n\n")); // End of struct

                /* Declarations */
                append(&sb, tprint(S("u64  %S__get_gpu_material_size(void);\n"), mtl.name));
                append(&sb, tprint(S("void %S__write_gpu_material(Material_Base *material, void *dst);\n"), mtl.name));
                append(&sb, tprint(S("void %S__init();\n\n"), mtl.name));


                /* Definitions. These go after the include guard, in the implementation section. */
                append(&impl, tprint(S("u64 %S__get_gpu_material_size(void) {\n"), mtl.name));
                append(&impl, tprint(S("  return sizeof(%S::GPU_Material);\n"), mtl.name));
                append(&impl, S("}\n\n"));

                append(&impl, tprint(S("void %S__write_gpu_material(Material_Base *material, void *dst) {\n"), mtl.name));
                append(&impl, tprint(S("  %S *m = (%S *)material;\n"), mtl.name, mtl.name));
                append(&impl, tprint(S("  %S::GPU_Material *gpu = (%S::GPU_Material *)dst;\n"), mtl.name, mtl.name));
                for (u32 i = 0; i < mtl.num_fields; ++i) {
                    Shader_Field field = mtl.fields[i];
                    String name = field.name;
                    if (field.attribute == S("Texture")) {
                        append(&impl, tprint(S("  gpu->%S = gfx_srv_bindless_from_texture(m->%S);\n"), name, name));
                    } else {
                        append(&impl, tprint(S("  gpu->%S = m->%S;\n"), name, name));
                    }
                }
                append(&impl, S("}\n\n"));

                append(&impl, tprint(S("void %S__init() {\n"), mtl.name));
                append(&impl, S("  Material_Type_Info info = {};\n"));
                append(&impl, tprint(S("  info.base.get_gpu_material_size = %S__get_gpu_material_size;\n"), mtl.name));
                append(&impl, tprint(S("  info.base.write_gpu_material    = %S__write_gpu_material;\n"), mtl.name));
                append(&impl, tprint(S("  info.cpu_size   = sizeof(%S);\n"), mtl.name));
                append(&impl, tprint(S("  info.num_fields = %d;\n"), mtl.num_fields));
                for (u32 i = 0; i < mtl.num_fields; ++i) {
                    Shader_Field field = mtl.fields[i];
                    String type = S("MATERIAL_FIELD_SCALAR");
                    if (field.attribute == S("Texture")) {
                        type = S("MATERIAL_FIELD_ASSET");
                    }
                    append(&impl, tprint(S("  info.fields[%d] = { %S, S(\"%S\"), sizeof(%S::%S), offset_of(%S, %S) };\n"),
                                         i, type, field.name, mtl.name, field.name, mtl.name, field.name));
                }
                append(&impl, tprint(S("  table_add(&material_type_table, guid_from_string(S(\"%S\")), info);\n"), asset_path));
                append(&impl, S("}\n\n"));

            } else {
                log_warning(S("Unrecognized extension '%S' from '%S' in material shader directory: %S"), 
                            ext, path, material_shader_dir);
            }
        } else {
            log_error(S("Failed to get extension from '%S'"), path);
        }
    }

    append(&impl, S("void init_material_type_table() {\n"));
    for (String name : names) {
        append(&impl, tprint(S("  %S__init();\n"), name));
    }
    append(&impl, S("}\n"));


    /* Include Guard End */
    append(&sb, S("#endif // RTS_GENERATED_MATERIAL_H\n\n"));


    /* Implementation section. Its own guard, so include order doesn't matter:
       the declarations above may already have been pulled in by another header. */
    append(&sb, S(R"GEN(#if defined(GENERATED_MATERIAL_IMPLEMENTATION) && !defined(RTS_GENERATED_MATERIAL_IMPL)
#define RTS_GENERATED_MATERIAL_IMPL

)GEN"));
    append(&sb, flush(&impl));
    append(&sb, S("\n#endif // GENERATED_MATERIAL_IMPLEMENTATION\n"));


    // Let's actually write.
    // @Speed: Write straight to file from string builder.
    String s = flush(&sb);

    // Write to file
    String filename = S("material.h");
    String path = tprint(S("%S/%S"), output_dir, filename);
    File file = file_open(path, true, false);
    if ( !file_is_valid(file) ) {
        return;
    }

    file_write(file, s.str, s.len);
    file_close(&file);

    log_info(S("Generated code for %llu materials."), fl.count);
}

