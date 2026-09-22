// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_ASSET_MESH_H
#define RTS_ASSET_MESH_H

#include "basic/core.h"
#include "basic/allocator.h"
#include "basic/string.h"
#include "math/math.h"
#include "os/os.h"

#define MAX_BONE_PER_VERTEX 4

namespace Asset
{
    // Mirrored by 'struct Vertex' in data/shaders/pass/surface.slang. Keep the two in sync.
    // The vertex buffer is bound as a StructuredBuffer, so the stride is this struct's size.
    //
    // Members are ordered so every 4-component vector sits on a 16-byte boundary, which
    // makes the layout identical under both scalar and cbuffer-style packing rules. It
    // also has to be: 'vec4' is a '__m128' underneath, so it's 16-aligned regardless.
    struct Vertex {
        vec4 tangent;                           // 0   w: sign
        vec3 position;                          // 16
        u32  color;                             // 28  packed RGBA8
        vec3 normal;                            // 32
        f32  _pad0;                             // 44
        f32  node_weights[MAX_BONE_PER_VERTEX]; // 48
        s32  node_ids[MAX_BONE_PER_VERTEX];     // 64
        vec2 uv;                                // 80
        f32  _pad1[2];                          // 88
    };
    static_assert(sizeof(Vertex) == 96, "Asset::Vertex must match surface.slang's Vertex.");

    // A submesh. Vertex and index data don't outlive the load, they go straight to the GPU.
    // 'gpu_id' is what you put in Entity::mesh.
    struct Mesh {
        String name;
        Guid   gpu_id;
        u32    num_vertices;
        u32    num_indices;
    };

    struct Model {
        String name;
        u32    num_meshes;
        Mesh  *meshes;
    };

    // Parses a '.triangle_mesh' file and uploads every submesh to the GPU. The returned
    // model only holds metadata, and is allocated out of 'allocator'.
    void load_model(Model *out, String file_path, String short_name, Allocator allocator);

    Mesh *mesh_from_name(Model *model, String name);

    // Asset_Type_Info::load_proc for the 'triangle_mesh' extension.
    void mesh_load_proc(String filepath, String short_name, void *user_data);

    // Null if the model was never requested.
    Model *model_from_guid(Guid id);
}

#endif // RTS_ASSET_MESH_H
