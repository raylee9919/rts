// Copyright Seong Woo Lee. All Rights Reserved.

// C++
//
#include <string>
#include <unordered_map>

#include "third_party/ufbx/ufbx.h"
#include "third_party/ufbx/ufbx.c"

// .h
//
#include "third_party/xxhash3/xxhash.c"
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "asset/texture.h"
#include "asset.h"
#include "third_party/mikktspace/mikktspace.h"
#include "fbx_importer_util.h"
#include "fbx_importer.h"

// .cpp
//
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"

#pragma warning(push)
#pragma warning(disable : 4456)
#include "third_party/mikktspace/mikktspace.c"
#pragma warning(pop)

#include "fbx_importer_util.cpp"

static int a;
static int b;


void fbx_fill_meshes_recursively(State *state, ufbx_node *node)
{
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    Arena *arena = state->scene_arena;

    ufbx_mesh *fbx_mesh = node->mesh;

    if (fbx_mesh &&
        node->attrib_type == UFBX_ELEMENT_MESH && 
        node->visible) 
    {
        bool has_uv      = fbx_mesh->vertex_uv.exists;
        bool has_color   = fbx_mesh->vertex_color.exists;
        bool has_tangent = fbx_mesh->vertex_tangent.exists;

        size_t num_tri = fbx_mesh->num_triangles;
        Asset_Vertex *vertices = push_array(arena, Asset_Vertex, num_tri * 3); // @Correctness
        size_t num_vert = 0;

        Asset_Mesh mesh = {};

        // Get name.
        mesh.length = (u32)node->name.length;
        mesh.name   = push_array(arena, u8, mesh.length + 1);
        memcpy(mesh.name, node->name.data, sizeof(u8)*mesh.length);
        mesh.name[mesh.length] = 0;

        size_t tri_num_indices = fbx_mesh->max_face_triangles * 3;
        u32 *tri_indices = push_array(scratch.arena, u32, tri_num_indices);

        // @Temporary
        // @Temporary
        // @Temporary
        // @Temporary
        // @Temporary
        //m4x4 *global_transforms = new m4x4[state->num_bones];
        //memset(global_transforms, 0, sizeof(m4x4) * state->num_bones);

        //for (u32 i = 0; i < state->num_bones; ++i) {
        //    auto *bone = &state->bones[i];
        //    s32 parent = bone->parent;
        //    if (parent >= 0) {
        //        global_transforms[i] = global_transforms[parent] * bone->local_transform;
        //    } else {
        //        global_transforms[i] = bone->local_transform;
        //    }
        //}
        // @Temporary
        // @Temporary
        // @Temporary
        // @Temporary
        // @Temporary

        auto faces = fbx_mesh->faces;
        for (size_t fi = 0; fi < faces.count; ++fi) {
            ufbx_face face = faces.data[fi];

            u32 face_num_tri = ufbx_triangulate_face(tri_indices, tri_num_indices, fbx_mesh, face);

            for (size_t ti = 0; ti < face_num_tri; ++ti) {
                for (size_t vi = 0; vi < 3; ++vi) {
                    u32 index = tri_indices[ti*3 + vi];

                    auto *vert = &vertices[num_vert++];

                    if (!fbx_mesh->skinned_is_local) {
                        vert->pos     = to_v3(ufbx_get_vertex_vec3(&fbx_mesh->skinned_position, index));
                        vert->normal  = to_v3(ufbx_get_vertex_vec3(&fbx_mesh->skinned_normal, index));
                    } else {
                        vert->pos     = to_v3(ufbx_transform_position(&node->geometry_to_world, ufbx_get_vertex_vec3(&fbx_mesh->skinned_position, index)));
                        vert->normal  = to_v3(ufbx_transform_direction(&node->geometry_to_world, ufbx_get_vertex_vec3(&fbx_mesh->skinned_normal, index)));
                    }

                    // Because we want to exclude a chain of transforms before the root bone.
                    vert->pos     = (state->undo_pre_root_bone_transforms * V4(vert->pos, 1.f)).xyz;
                    vert->normal  = normalize((state->undo_pre_root_bone_transforms * V4(vert->normal, 0.f)).xyz);


                    // @Temporary
                    // @Temporary
                    // @Temporary
                    // @Temporary
                    // @Temporary
                    //std::string key((char *)mesh.name, mesh.length);
                    //s32 id = state->bone_map[key];
                    //vert->pos = (global_transforms[id] * V4(vert->pos, 1.f)).xyz;
                    // @Temporary
                    // @Temporary
                    // @Temporary
                    // @Temporary
                    // @Temporary


                    if (has_uv) {
                        vert->uv = to_v2(ufbx_get_vertex_vec2(&fbx_mesh->vertex_uv, index));
                    } else {
                        vert->uv = {};
                    }

                    // @Todo: Currently ignoring the color, because sometimes it gives a null color and we 
                    // are interpreting vertex color as a tint in our shader.
                    vert->color = v4(1.f);

                    //if (has_tangent) {
                    //    vert->tangent.xyz = to_v3(ufbx_get_vertex_vec3(&fbx_mesh->vertex_tangent, index));
                    //    vert->tangent.w = 1.f; // @Fix
                    //    assert(0);
                    //}

                    // Init skinning data.
                    for (int i = 0; i < MAX_BONE_PER_VERTEX; ++i) vert->node_ids[i] = -1;
                    for (int i = 0; i < MAX_BONE_PER_VERTEX; ++i) vert->node_weights[i] = 0.f;

                    // @Temporary
                    if (fbx_mesh->skin_deformers.count > 0) {
                        assert(fbx_mesh->skin_deformers.count == 1);

                        u32 vert_idx = fbx_mesh->vertex_indices.data[index];

                        ufbx_skin_deformer *skin = fbx_mesh->skin_deformers.data[0];
                        ufbx_skin_vertex skin_vertex = skin->vertices.data[vert_idx];

                        int num_weights = min((int)skin_vertex.num_weights, MAX_BONE_PER_VERTEX);
                        f32 total_weight = 0.f;

                        for (int i = 0; i < num_weights; ++i) {
                            ufbx_skin_weight skin_weight = skin->weights.data[skin_vertex.weight_begin + i];
                            ufbx_skin_cluster *cluster = skin->clusters.data[skin_weight.cluster_index];
                            ufbx_node *bone_node = cluster->bone_node;
                            ufbx_string bone_name = bone_node->name;

                            vert->node_ids[i]     = state->bone_map[std::string(bone_name.data, bone_name.length)];
                            vert->node_weights[i] = (f32)skin_weight.weight;

                            total_weight += vert->node_weights[i];
                        }

                        // Normalize weights.
                        f32 rcp_weights = 0.f;
                        if (total_weight > 1e-8) {
                            rcp_weights = 1.f / total_weight;
                        }
                        for (int i = 0; i < num_weights; ++i) {
                            vert->node_weights[i] *= rcp_weights;
                        }
                    }
                }

            }
        }

        assert(num_vert == num_tri * 3);

        ufbx_vertex_stream streams[1] = {
            { vertices, num_vert, sizeof(vertices[0]) },
        };
        size_t num_indices = num_tri * 3;
        u32 *indices = push_array(arena, u32, num_indices); // @Correctness

        // This'll deduplicate vertices, modifying the arrays passed in 'streams[]',
        // indices are written in 'indices[]' and the number of unique vertices is returned.
        num_vert = ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

        mesh.vertices = vertices;
        mesh.vertex_count = (u32)num_vert;

        mesh.indices = indices;
        mesh.index_count = (u32)num_indices;

        // @Temporary
        if (1) {
            state->mikkt_ctx.m_pUserData = &mesh;
            genTangSpaceDefault(&state->mikkt_ctx);
        }

        
        // Push the mesh.
        state->meshes[state->num_meshes++] = mesh;
    }

    // Traverse down.
    ufbx_node_list children = node->children;
    for (size_t i = 0; i < children.count; ++i) {
        fbx_fill_meshes_recursively(state, children.data[i]);
    }
}

void fbx_fill_meshes(State *state)
{
    u32 max_num_meshes = (u32)state->scene->meshes.count;
    state->meshes = push_array(state->scene_arena, Asset_Mesh, max_num_meshes);

    fbx_fill_meshes_recursively(state, state->scene->root_node);
}

ufbx_node *fbx_find_root_bone(ufbx_node *node)
{
    if (node->attrib_type == UFBX_ELEMENT_BONE) {
        ufbx_bone *bone = node->bone;
        if (bone) {
            if (bone->is_root) {
                return node;
            }
        }
    }

    ufbx_node_list children = node->children;
    for (int i = 0; i < (int)children.count; ++i) {
        ufbx_node *bone = fbx_find_root_bone(children.data[i]);
        if (bone) {
            return bone;
        }
    }

    return NULL;
}

void fbx_fill_bones_recursively(State *state, ufbx_node *node)
{
    Temporary_Arena scratch = scratch_begin();
    defer(scratch_end(scratch));

    const s32 id = state->num_bones;

    Arena *arena = state->scene_arena;
    auto *bone = &state->bones[id];
    ufbx_bone *fbx_bone = node->bone;

    if (fbx_bone && node->attrib_type == UFBX_ELEMENT_BONE) {
        ++state->num_bones;

        // Get name
        ufbx_string name = node->name;
        bone->name = push_array(arena, u8, name.length + 1);
        bone->length = (u32)name.length;
        memcpy(bone->name, name.data, sizeof(u8)*name.length);
        bone->name[name.length] = 0;

        // Get local matrix. (to parent bone space)
        bone->local_transform = to_m4x4(node->node_to_parent);

        // Get parent
        if (id == 0) {
            bone->parent = -1;
        } else {
            ufbx_node *parent_node = node->parent;
            ufbx_string parent_name = parent_node->name;
            std::string key(parent_name.data, parent_name.length);
            assert(state->bone_map.find(key) != state->bone_map.end());
            bone->parent = state->bone_map[key];
        }

        // Fill in the bone name to id hash map.
        std::string key = std::string(name.data, name.length);
        assert(state->bone_map.find(key) == state->bone_map.end());
        state->bone_map[key] = id;

        // Traverse down.
        ufbx_node_list children = node->children;
        for (int i = 0; i < (int)children.count; ++i) {
            fbx_fill_bones_recursively(state, children.data[i]);
        }
    } else {
        assert(!"This node is supposed to be a bone!");
    }
}

void fbx_fill_bones(State *state)
{
    ufbx_scene *scene = state->scene;
    u32 num_bones = (u32)scene->bones.count;
    if (num_bones == 0) return;

    ufbx_node *root_bone_node = fbx_find_root_bone(scene->root_node);
    assert(root_bone_node);


    // 
    {
        m4x4 chain = identity();
        m4x4 undo  = identity();
        for (ufbx_node *x = root_bone_node->parent;;x = x->parent) {
            if (x) {
                m4x4 m = to_m4x4(x->node_to_parent);
                chain = m * chain;
                undo = inverse(m) * undo;
            }
            if (x->is_root) break;
        }
        state->root_transform = chain;
        state->undo_pre_root_bone_transforms = undo;
    }

    // We now have the actual bone count.
    state->bones = push_array(state->scene_arena, FBXI_Bone, num_bones);


    // Recursively fill in the bones starting from the "actual" root bone node.
    fbx_fill_bones_recursively(state, root_bone_node);

    // Does the value match the expected value?
    assert(state->num_bones == num_bones);


    // Calc inverse bind poses.
    //
    for (u32 bi = 0; bi < state->num_bones; ++bi) {
        auto *bone = &state->bones[bi];
        s32 parent = bone->parent;
        if (parent >= 0) {
            bone->inverse_bind_pose = state->bones[parent].inverse_bind_pose * bone->local_transform;
        } else {
            bone->inverse_bind_pose = bone->local_transform;
        }
    }

    for (u32 bi = 0; bi < state->num_bones; ++bi) {
        auto *bone = &state->bones[bi];
        bone->inverse_bind_pose = inverse(bone->inverse_bind_pose);
    }
}

void fbx_fill_animation(State *state, int index)
{
    Arena *arena = state->scene_arena;

    Asset_Animation *out_anim = &state->anims[index];
    ufbx_scene *scene = state->scene;
    ufbx_anim_stack *anim_stack = scene->anim_stacks[index];
    ufbx_anim *anim = anim_stack->anim;

    ufbx_baked_anim *baked = ufbx_bake_anim(scene, anim, NULL, NULL);
    assert(baked);

    ufbx_string anim_name = anim_stack->name;
    out_anim->length = (u32)anim_name.length;
    out_anim->name = push_array(arena, u8, anim_name.length + 1);
    memcpy(out_anim->name, anim_name.data, sizeof(u8)*anim_name.length);

    f32 duration = (f32)(anim_stack->time_end - anim_stack->time_begin);
    out_anim->duration = duration;

    u32 num_nodes = state->num_bones; // @Todo: This isn't right...technically...
    out_anim->num_nodes = num_nodes;
    out_anim->nodes = push_array(arena, Asset_Animation_Node, num_nodes);

    u32 num_samples = 0;
    for (u32 i = 0; i < (u32)baked->nodes.count; ++i) {
        ufbx_baked_node *node = &baked->nodes[i];
        num_samples = max(num_samples, (u32)node->translation_keys.count);
        num_samples = max(num_samples, (u32)node->rotation_keys.count);
        num_samples = max(num_samples, (u32)node->scale_keys.count);
    }
    out_anim->num_samples = num_samples;

    assert(num_samples >= 2);
    f32 dt = duration / (num_samples - 1);

    u32 tmp = 0;

    for (int ni = 0, fill_idx = 0; ni < (int)baked->nodes.count; ++ni) {
        ufbx_baked_node *baked_node = &baked->nodes[ni];
        ufbx_node *scene_node = scene->nodes[baked_node->typed_id];

        std::string key = std::string(scene_node->name.data);
        if (state->bone_map.find(key) != state->bone_map.end()) { // key found
            tmp++;

            auto *out_node = &out_anim->nodes[fill_idx++];
            out_node->id = state->bone_map[key];

            out_node->translations = push_array(arena, v3, num_samples);
            out_node->rotations    = push_array(arena, Quaternion, num_samples);
            out_node->scales       = push_array(arena, v3, num_samples);

            for (u32 si = 0; si < num_samples; ++si) {
                f32 t = anim_stack->time_begin + dt * (f32)si;
                ufbx_transform trs = ufbx_evaluate_transform(anim, scene_node, t);
                out_node->translations[si] = to_v3(trs.translation);
                out_node->rotations[si]    = to_quaternion(trs.rotation);
                out_node->scales[si]       = to_v3(trs.scale);
            }
        }
    }

    assert(tmp == num_nodes);

    ufbx_free_baked_anim(baked);
}

void fbx_fill_animations(State *state)
{
    ufbx_scene *scene = state->scene;

    int num_anims = (int)scene->anim_stacks.count;
    printf("Number of animations: %d\n", num_anims);

    state->anims = push_array(state->scene_arena, Asset_Animation, num_anims);
    for (int i = 0; i < num_anims; ++i) {
        fbx_fill_animation(state, i);
    }
    state->num_anims = num_anims;
}

int mikkt_get_num_faces(const SMikkTSpaceContext *ctx)
{
    // @Robustness: I'm just assuming all faces are triangulated.
    auto *mesh = (Asset_Mesh *)ctx->m_pUserData;
    assert(mesh->index_count % 3 == 0);
    return mesh->index_count / 3;
}

int mikkt_get_num_vertices_of_face(const SMikkTSpaceContext *ctx, const int face)
{
    // @Robustness: I'm just assuming all faces are triangulated.
    return 3;
}

void mikkt_get_position(const SMikkTSpaceContext *ctx, float out[], const int face, const int vert)
{
    auto *mesh = (Asset_Mesh *)ctx->m_pUserData;

    u32 index = mesh->indices[face*3 + vert];
    v3 p = mesh->vertices[index].pos;

    out[0] = p.x;
    out[1] = p.y;
    out[2] = p.z;
}

void mikkt_get_normal(const SMikkTSpaceContext *ctx, float out[], const int face, const int vert)
{
    auto *mesh = (Asset_Mesh *)ctx->m_pUserData;

    u32 index = mesh->indices[face*3 + vert];
    v3 n = mesh->vertices[index].normal;

    out[0] = n.x;
    out[1] = n.y;
    out[2] = n.z;
}

void mikkt_get_uv(const SMikkTSpaceContext *ctx, float out[], const int face, const int vert)
{
    auto *mesh = (Asset_Mesh *)ctx->m_pUserData;

    u32 index = mesh->indices[face*3 + vert];
    v2 uv = mesh->vertices[index].uv;

    out[0] = uv.x;
    out[1] = uv.y;
}

void mikkt_set_basic(const SMikkTSpaceContext *ctx, const float tangent[], const float sign, const int face, const int vert)
{
    auto *mesh = (Asset_Mesh *)ctx->m_pUserData;

    u32 index = mesh->indices[face*3 + vert];
    auto *v = &mesh->vertices[index];

    v->tangent.x = tangent[0];
    v->tangent.y = tangent[1];
    v->tangent.z = tangent[2];
    v->tangent.w = sign;
}


int main()
{
    os_init();
    thread_init();

    State *state;
    {
        Arena *arena = arena_alloc();
        state = new State; // @Temporary: Until we remove C++ hash table...
        state->arena = arena;
        state->scene_arena = arena_alloc();

        // @Temporary: Ditto
        state->num_meshes = 0;
        state->num_bones = 0;

        state->root_transform = identity();
        state->undo_pre_root_bone_transforms = identity();

        auto *ctx = &state->mikkt_ctx;
        ctx->m_pInterface = push_struct(arena, SMikkTSpaceInterface);
        ctx->m_pInterface->m_getNumFaces = mikkt_get_num_faces;
        ctx->m_pInterface->m_getNumVerticesOfFace = mikkt_get_num_vertices_of_face;
        ctx->m_pInterface->m_getPosition = mikkt_get_position;
        ctx->m_pInterface->m_getNormal = mikkt_get_normal;
        ctx->m_pInterface->m_getTexCoord = mikkt_get_uv;
        ctx->m_pInterface->m_setTSpaceBasic = mikkt_set_basic;
    }


    {
        Utf8 name     = utf8lit("castle");
        Utf8 in_file  = utf8f(state->scene_arena, "C:/dev/rts/data/input/%S.fbx", name);

        Utf8 out_mesh = utf8f(state->scene_arena, "C:/dev/rts/data/%S.triangle_mesh", name);
        Utf8 out_skel = utf8f(state->scene_arena, "C:/dev/rts/data/%S.skeleton", name);
        Utf8 out_anim = utf8f(state->scene_arena, "C:/dev/rts/data/%S.keyframed_animation", name);

        ufbx_load_opts opts = {};
        opts.target_axes                 = ufbx_axes_right_handed_y_up;
        opts.target_unit_meters          = 1.0f;
        opts.generate_missing_normals    = true;
        opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY; 
        opts.inherit_mode_handling       = UFBX_INHERIT_MODE_HANDLING_IGNORE; // This is what ufbx suggests. "...what many importers do and simplifies everything."
        opts.evaluate_skinning           = true;
        opts.clean_skin_weights          = true;
        opts.normalize_normals           = true;

        ufbx_error error;
        ufbx_scene *scene = ufbx_load_file((const char *)in_file.str, &opts, &error);

        if (!scene) {
            printf("ERROR: Failed to load file '%.*s'. %s\n", (int)in_file.len, (const char *)in_file.str, error.description.data);
            return -1;
        }

        state->scene = scene;

        fbx_print_nodes(scene->root_node);
        fbx_fill_bones(state);
        fbx_fill_meshes(state);
        fbx_fill_animations(state);



        // Write skeleton.
        //
#if 1
        if (state->num_bones > 0) {
            FILE *f = fopen((const char *)out_skel.str, "wb");
            if (!f) {
                printf("ERROR: Failed to open file '%.*s'.\n", (int)out_skel.len, (const char *)out_skel.str);
                return -1;
            }

            fprintf(f, "%u\n", state->num_bones);
            fprintf(f, "\n");

            m4x4 m = state->root_transform;
            fprintf(f, "%.6f %.6f %.6f %.6f\n", m._11, m._12, m._13, m._14);
            fprintf(f, "%.6f %.6f %.6f %.6f\n", m._21, m._22, m._23, m._24);
            fprintf(f, "%.6f %.6f %.6f %.6f\n", m._31, m._32, m._33, m._34);
            fprintf(f, "%.6f %.6f %.6f %.6f\n", m._41, m._42, m._43, m._44);
            fprintf(f, "\n");

            for (u32 i = 0; i < state->num_bones; ++i) {
                auto bone = state->bones[i];
                fprintf(f, "%u %.*s\n", bone.length, bone.length, bone.name);
                fprintf(f, "%d\n", bone.parent);

                m4x4 l = bone.local_transform;
                fprintf(f, "%.6f %.6f %.6f %.6f\n", l._11, l._12, l._13, l._14);
                fprintf(f, "%.6f %.6f %.6f %.6f\n", l._21, l._22, l._23, l._24);
                fprintf(f, "%.6f %.6f %.6f %.6f\n", l._31, l._32, l._33, l._34);
                fprintf(f, "%.6f %.6f %.6f %.6f\n", l._41, l._42, l._43, l._44);
                fprintf(f, "\n");

                m4x4 inv = bone.inverse_bind_pose;
                fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._11, inv._12, inv._13, inv._14);
                fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._21, inv._22, inv._23, inv._24);
                fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._31, inv._32, inv._33, inv._34);
                fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._41, inv._42, inv._43, inv._44);
                fprintf(f, "\n");
            }

            fclose(f);
        }
#endif

        // Write mesh.
        //
#if 1
        if (state->num_meshes > 0) {
            int ver_major = 0;
            int ver_minor = 1;
            int ver_patch = 0;

            FILE *f = fopen((const char *)out_mesh.str, "wb");
            if (!f) {
                printf("ERROR: Failed to open file '%.*s'.\n", (int)out_mesh.len, (const char *)out_mesh.str);
                return -1;
            }

            fprintf(f, "v%d.%d.%d\n\n", ver_major, ver_minor, ver_patch);

            fprintf(f, "%u\n\n", state->num_meshes);

            for (u32 mi = 0; mi < state->num_meshes; ++mi) {
                auto *mesh = &state->meshes[mi];

                fprintf(f, ";%.*s\n", (int)mesh->length, (const char *)mesh->name);

                fprintf(f, "%u\n", mesh->vertex_count);
                for (u32 vi = 0; vi < mesh->vertex_count; ++vi) {
                    Asset_Vertex *vert = &mesh->vertices[vi];
                    fprintf(f, "%.6f %.6f %.6f\n", vert->pos.x, vert->pos.y, vert->pos.z);
                    fprintf(f, "%.6f %.6f %.6f\n", vert->normal.x, vert->normal.y, vert->normal.z);
                    fprintf(f, "%.6f %.6f\n", vert->uv.x, vert->uv.y);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", vert->color.r, vert->color.g, vert->color.b, vert->color.a);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", vert->tangent.x, vert->tangent.y, vert->tangent.z, vert->tangent.w);
                    for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) {
                        fprintf(f, "%d ", vert->node_ids[i]);
                    }
                    fprintf(f, "\n");

                    for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) {
                        fprintf(f, "%.6f ", vert->node_weights[i]);
                    }
                    fprintf(f, "\n");

                    fprintf(f, "\n");
                }

                fprintf(f, "%u\n", mesh->index_count);
                for (u32 i = 0; i < mesh->index_count; ++i) {
                    fprintf(f, "%u ", mesh->indices[i]);

                    if ((i+1) % 6 == 0) {
                        fprintf(f, "\n");
                    }
                }
                fprintf(f, "\n");

            }

            fclose(f);
        }
#endif

        // Write animations.
#if 0
        assert(state->num_anims == 1);
        for (int i = 0; i < state->num_anims; ++i) {
            auto *anim = &state->anims[i];

            FILE *f = fopen((char *)out_anim.str, "wb");
            assert(f);
            {
                fprintf(f, "%u %.*s\n", anim->length, anim->length, anim->name);
                fprintf(f, "%.6f\n", anim->duration);
                fprintf(f, "%u\n", anim->num_samples);
                fprintf(f, "%u\n\n", anim->num_nodes);

                for (u32 ni = 0; ni < anim->num_nodes; ++ni) {
                    auto* node = &anim->nodes[ni];

                    fprintf(f, "%d\n", node->id);

                    for (u32 i = 0; i < anim->num_samples; ++i) {
                        fprintf(f, "%.6f %.6f %.6f\n", node->translations[i].x, node->translations[i].y, node->translations[i].z);
                        fprintf(f, "%.6f %.6f %.6f %.6f\n", node->rotations[i].w, node->rotations[i].x, node->rotations[i].y, node->rotations[i].z);
                        fprintf(f, "%.6f %.6f %.6f\n\n", node->scales[i].x, node->scales[i].y, node->scales[i].z);
                    }
                }
            }
            fclose(f);
        }
#endif

        arena_clear(state->scene_arena);
        ufbx_free_scene(scene);
    }


    printf("***SUCCESSFUL!***\n");
    return 0;
}
