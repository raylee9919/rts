// Copyright Seong Woo Lee. All Rights Reserved.

#include "asset/mesh.h"
#include "asset/asset.h"
#include "asset/parser.h"
#include "basic/context.h"
#include "basic/hash_table.h"
#include "basic/log.h"
#include "gfx/gfx.h"
#include "os/os.h"

namespace Asset
{
    static Table<Guid, Model, hash_guid> model_table;
    static b32 model_table_initted = false;

    void load_model(Model *out, String file_path, String short_name, Allocator allocator)
    {
        R_ASSERT(out);

        // The file is text, and a character at a time is all this parser does. Everything
        // transient - the file itself, the vertex and index arrays - lives in scratch and
        // is gone by the time we return. Only the metadata below survives.
        Temporary_Arena scratch = scratch_begin();
        defer( scratch_end(scratch) );

        String contents = read_entire_file(file_path, tctx.temp);
        if ( !contents.str ) {
            log_error(S("Couldn't read mesh file: '%S'"), file_path);
            return;
        }

        Parser p = {};
        init(&p, contents.str, contents.len);

        Version ver = parse_version(&p);

        u32 num_meshes = parse_u32(&p);

        out->name       = copy_string(short_name, allocator);
        out->num_meshes = num_meshes;
        out->meshes     = (Mesh *)alloc(sizeof(Mesh) * num_meshes, allocator);

        log_info(S("Loading model '%S', %u meshes."), short_name, num_meshes);

        u32 num_total_vertices = 0;
        u32 num_total_indices  = 0;

        for (u32 mi = 0; mi < num_meshes; ++mi)
        {
            Mesh *mesh = &out->meshes[mi];

            if (ver.major >= 0 && ver.minor >= 1) {
                eat_whitespace(&p);
                R_ASSERT( peek(&p) == ';' );
                eat(&p);
                mesh->name = copy_string(parse_string_by_line(&p, scratch.arena), allocator);
            } else {
                u8 name_len = (u8)parse_u32(&p);
                mesh->name = copy_string(parse_string_by_length(&p, name_len, scratch.arena), allocator);
            }

            // Vertices
            //
            u32 num_vertices = parse_u32(&p);
            Vertex *vertices = push_array(scratch.arena, Vertex, num_vertices);

            for (u32 vi = 0; vi < num_vertices; ++vi) {
                Vertex *vert = &vertices[vi];

                vert->position = parse_v3(&p);
                vert->normal   = parse_v3(&p);
                vert->uv       = parse_v2(&p);

                vec4 color = parse_v4(&p);
                u32 r = (u32)(color.r * 255.f + 0.5f);
                u32 g = (u32)(color.g * 255.f + 0.5f);
                u32 b = (u32)(color.b * 255.f + 0.5f);
                u32 a = (u32)(color.a * 255.f + 0.5f);
                vert->color = r | (g << 8) | (b << 16) | (a << 24);

                vert->tangent = parse_v4(&p);

                for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i)  vert->node_ids[i]     = parse_s32(&p);
                for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i)  vert->node_weights[i] = parse_f32(&p);
            }

            // Indices
            //
            u32 num_indices = parse_u32(&p);
            u32 *indices = push_array(scratch.arena, u32, num_indices);

            for (u32 ii = 0; ii < num_indices; ++ii) {
                indices[ii] = parse_u32(&p);
            }

            mesh->num_vertices = num_vertices;
            mesh->num_indices  = num_indices;
            mesh->gpu_id       = guid_from_string(tprint(S("%S#%S"), short_name, mesh->name));

            gfx_mesh_create(mesh->gpu_id,
                            vertices, num_vertices, sizeof(Vertex),
                            indices,  num_indices,  sizeof(indices[0]));

            log_info(S("  Mesh #%u '%S': %u vertices, %u indices."), mi, mesh->name, num_vertices, num_indices);

            num_total_vertices += num_vertices;
            num_total_indices  += num_indices;
        }

        log_info(S("  Total: %u vertices, %u indices, %u triangles."),
                 num_total_vertices, num_total_indices, num_total_indices / 3);

        R_ASSERT( is_eof(&p) );
    }

    Mesh *mesh_from_name(Model *model, String name)
    {
        for (u32 i = 0; i < model->num_meshes; ++i) {
            if (model->meshes[i].name == name)  return &model->meshes[i];
        }
        return nullptr;
    }

    void mesh_load_proc(String filepath, String short_name, void *user_data)
    {
        Allocator heap = asset_system->heap;

        if ( !model_table_initted ) {
            model_table.allocator = heap;
            model_table_initted   = true;
        }

        Model model = {};
        load_model(&model, filepath, short_name, heap);

        table_add(&model_table, guid_from_string(short_name), model);
    }

    Model *model_from_guid(Guid id)
    {
        if ( !model_table_initted )  return nullptr;
        return table_find_pointer(&model_table, id);
    }
}
