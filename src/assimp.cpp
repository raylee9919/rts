/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// --------------------------------------
// @Note: Assimp includes.
#include "vendor/assimp/Importer.hpp"
#include "vendor/assimp/scene.h"
#include "vendor/assimp/postprocess.h"

// --------------------------------------
// @Note: [.h]
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_math.h"
#include "rts_asset.h"

#define STB_IMAGE_IMPLEMENTATION
#include "vendor/stb_image.h"

#include "assimp.h"

// --------------------------------------
// @Note: [.cpp]
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"
#include "rts_math.cpp"




#define ASSIMP_PRINT_NODES              1
#define ASSIMP_PRINT_NODE_TRASNFORM     0

internal v3
aiv3_to_v3(aiVector3D ai_v) 
{
    v3 v = {};
    v.x = ai_v.x;
    v.y = ai_v.y;
    v.z = ai_v.z;
    return v;
}

internal Quaternion
aiqt_to_qt(aiQuaternion ai_q) 
{
    Quaternion q = {};
    q.w = ai_q.w;
    q.x = ai_q.x;
    q.y = ai_q.y;
    q.z = ai_q.z;
    return q;
}

internal m4x4
ai_m4x4_to_m4x4(aiMatrix4x4 ai_mat) 
{
    m4x4 mat = {};
    mat.e[0][0] = ai_mat.a1;
    mat.e[0][1] = ai_mat.a2;
    mat.e[0][2] = ai_mat.a3;
    mat.e[0][3] = ai_mat.a4;

    mat.e[1][0] = ai_mat.b1;
    mat.e[1][1] = ai_mat.b2;
    mat.e[1][2] = ai_mat.b3;
    mat.e[1][3] = ai_mat.b4;

    mat.e[2][0] = ai_mat.c1;
    mat.e[2][1] = ai_mat.c2;
    mat.e[2][2] = ai_mat.c3;
    mat.e[2][3] = ai_mat.c4;

    mat.e[3][0] = ai_mat.d1;
    mat.e[3][1] = ai_mat.d2;
    mat.e[3][2] = ai_mat.d3;
    mat.e[3][3] = ai_mat.d4;

    return mat;
}

internal umm
string_length_with_null(char *str) 
{
    return (string_length(str) + 1);
}

internal s32 g_node_count;
internal void
debug_print_nodes(aiNode *node, u32 depth) 
{
    g_node_count++;
    m4x4 transform = ai_m4x4_to_m4x4(node->mTransformation);

#if ASSIMP_PRINT_NODES
    printf("%*s", depth << 1, "");
    printf("(%u)%s\n", depth, node->mName.data);
#endif

#if ASSIMP_PRINT_NODE_TRASNFORM
    for (s32 r = 0; r < 4; ++r) {
        printf("%*s", depth << 1, "");
        for (s32 c = 0; c < 4; ++c) {
            printf("%.2f ", transform.e[r][c]);
        }
        printf("\n");
    }
#endif

    for (u32 i = 0; i < node->mNumChildren; ++i) {
        debug_print_nodes(node->mChildren[i], depth + 1);
    }
}

internal char *
create_output_model_filepath(char *in_filepath)
{
    char *begin = in_filepath;
    char *end   = in_filepath;
    for (char *at = in_filepath; *at; ++at) {
        if (*at == '/') {
            begin = at + 1;
        } else if (*at == '.') {
            end = at;
        }
    }
    int filename_length = end - begin;

    umm directory_length = string_length(PATH_TO_DATA_FROM_BUILD) + string_length(ASSET_MESH_DIRECTORY) + 1;
    char *directory = malloc_array(char, directory_length);
    _snprintf(directory, directory_length, "%s%s", PATH_TO_DATA_FROM_BUILD, ASSET_MESH_DIRECTORY);

    umm filepath_length = directory_length + filename_length + string_length(ASSET_MESH_FILE_FORMAT) + 1;

    char *result = malloc_array(char, filepath_length);
    _snprintf(result, filepath_length, "%s%.*s%s", directory, filename_length, begin, ASSET_MESH_FILE_FORMAT);

    return result;
}

internal void
print_scene_abstract(const aiScene *scene)
{
    debug_print_nodes(scene->mRootNode, 0);
    printf("node count: %d\n", g_node_count);
    printf("  mesh count      : %d\n", scene->mNumMeshes);
    printf("  texture count   : %d\n", scene->mNumTextures);
    printf("  material count  : %d\n", scene->mNumMaterials);
    printf("  animation count : %d\n", scene->mNumAnimations);
}

internal u32
get_next_unfilled_bone_index(Asset_Vertex *asset_vertex)
{
    for (u32 idx = 0; idx < MAX_BONE_PER_VERTEX; ++idx) {
        if (asset_vertex->node_ids[idx] == -1) {
            return idx;
        }
    }
    return 2222;
}

struct Hash_Slot 
{
    char *name;
    s32 node_id; // this isn't the offset starting from the hash table slots!
    Hash_Slot *next;
};
struct Hash_Entry 
{
    Hash_Slot *first;
};
struct Hashmap 
{
    Hash_Entry *entries;
    umm length;
    s32 next_id;
};

internal u32
hash(char *key, u32 length) 
{
    u32 result = 0;
    for (char *c = key; *c; ++c) {
        result += *c;
    }
    result %= length;
    return result;
}

internal s32
id_from_name(char *name, Hashmap *hashmap)
{
    s32 id;
    u32 slot_idx = hash(name, hashmap->length);
    Hash_Entry *entry = hashmap->entries + slot_idx;
    Hash_Slot *slot = entry->first;

    if (slot) {
        for (;;) {
            if (string_equal(name, slot->name)) {
                id = slot->node_id;
                break;
            } else if (slot->next) {
                slot = slot->next;
            } else {
                Hash_Slot *new_slot = malloc_type(Hash_Slot);
                new_slot->node_id = hashmap->next_id++;
                new_slot->next = 0;
                new_slot->name = name;

                slot->next = new_slot;

                id = new_slot->node_id;
                break;
            }
        }
    } else {
        entry->first = malloc_type(Hash_Slot);
        entry->first->node_id = hashmap->next_id++;
        entry->first->next = 0;
        entry->first->name = name;

        id = entry->first->node_id;
    }

    return id;
}

internal void
swap_node(Asset_Node *nd1, Asset_Node *nd2)
{
    Asset_Node tmp = *nd1;
    *nd1 = *nd2;
    *nd2 = tmp;
}

// @TODO: Better sort, if slow.
internal void
sort_by_id(Asset_Node *nodes, u32 node_count)
{
    u32 i, j;
    b32 swapped;
    for (i = 0; i < node_count - 1; i++) {
        swapped = false;
        for (j = 0; j < node_count - i - 1; j++) {
            if (nodes[j].id > nodes[j + 1].id) {
                swap_node(&nodes[j], &nodes[j + 1]);
                swapped = true;
            }
        }

        if (swapped == false) {
            break;
        }
    }
}

internal void
fill_asset_nodes(const aiScene *model, Asset_Model *asset_model, Hashmap *hashmap)
{
    //
    // traverse through hierarchy, if certain name was in the hash-table,
    // that slot will give us the index of that node in node array.
    // If it wasn't, insert to the hash-table. Collision handling isn't much of
    // a big deal. Then, that slot will contain a 'next_to_write' number of 
    // the node array. Then increment 'next_to_write" by one.
    //
    aiNode *root = model->mRootNode;

    u32 debug_count = 0;
    u32 expected_node_count = asset_model->node_count;

    Asset_Node *asset_nodes = asset_model->nodes;

    aiNode *nodes[500];
    nodes[0] = root;
    u32 next_to_visit = 0;
    u32 next_to_write = 1;

    while (next_to_visit != next_to_write) {
        aiNode *node = nodes[next_to_visit];
        for (u32 i = 0; i < node->mNumChildren; ++i) {
            nodes[next_to_write++] = node->mChildren[i];
        }
        ++next_to_visit;
    }

    u32 node_count = next_to_write;
    assert(node_count == expected_node_count);
    next_to_write = 0;
    for (u32 i = 0; i < node_count; ++i) {
        aiNode *node = nodes[i];
        Asset_Node *asset_node = asset_nodes + next_to_write++;

        asset_node->id = id_from_name(node->mName.data, hashmap);
        asset_node->offset = identity();
        asset_node->transform = ai_m4x4_to_m4x4(node->mTransformation);
        asset_node->child_count = node->mNumChildren;
        asset_node->child_ids = malloc_array(s32, node->mNumChildren);
        for (u32 j = 0; j < node->mNumChildren; ++j) {
            asset_node->child_ids[j] = id_from_name(node->mChildren[j]->mName.data, hashmap);
        }
    }
    assert(next_to_write == expected_node_count);

    sort_by_id(asset_nodes, node_count);

    for (u32 mesh_idx = 0; mesh_idx < model->mNumMeshes; ++mesh_idx) {
        aiMesh *mesh = model->mMeshes[mesh_idx];
        for (u32 bone_idx = 0; bone_idx < mesh->mNumBones; ++bone_idx) {
            aiBone *bone = mesh->mBones[bone_idx];
            for (u32 idx = 0; idx < node_count; ++idx) {
                Asset_Node *asset_node = asset_nodes + idx;
                if (id_from_name(bone->mName.data, hashmap) == asset_node->id) {
                    asset_node->offset = ai_m4x4_to_m4x4(bone->mOffsetMatrix);
                    ++debug_count;
                    break;
                }
            }
        }
    }

    asset_model->root_bone_node_id = id_from_name(root->mName.data, hashmap);


    // @Debug
    for (s32 i = 0; i < (s32)node_count; ++i) {
        assert(asset_model->nodes[i].id == i);
    }
}

internal Asset_Texture *
load_texture(aiMaterial *material, aiTextureType type) 
{
    Asset_Texture *result = 0;

    if (material->GetTextureCount(type) > 0) {
        aiString filepath;
        material->GetTexture(type, 0, &filepath);
        const char *filename = get_filename_from_filepath(filepath.C_Str());
        assert(filename);

        result = malloc_type(Asset_Texture);

        int x, y, n;

        void *loaded_data;
        if (stbi_is_16_bit(filename)) {
            u16 *data = stbi_load_16(filename, &x, &y, &n, 0);
            Assert(data);
            loaded_data = data;
            result->bits_per_channel = 16;
            result->size = x*y*2*n;
            result->data = malloc_array(u8, result->size);
        } else {
            u8 *data = stbi_load(filename, &x, &y, &n, 0);
            Assert(data);
            loaded_data = data;
            result->bits_per_channel = 8;
            result->size = x*y*n;
            result->data = malloc_array(u8, result->size);
        }
        result->channel_count = n;
        result->width  = x;
        result->height = y;
        result->pitch  = x*n;
        memory_copy(result->data, loaded_data, result->size);

        stbi_image_free(loaded_data);
    }

    return result;
}

internal void
fill_asset_meshes(const aiScene *model, Asset_Model *asset_model, Hashmap *hashmap)
{
    u32 mesh_count = model->mNumMeshes;
    aiMesh **meshes = model->mMeshes;

    asset_model->mesh_count = mesh_count;
    asset_model->meshes     = malloc_array(Asset_Mesh, asset_model->mesh_count);

    for (u32 mesh_idx = 0;
         mesh_idx < mesh_count;
         ++mesh_idx)
    {
        Asset_Mesh *asset_mesh    = (asset_model->meshes + mesh_idx);
        aiMesh *mesh              = meshes[mesh_idx];

        u32 vertex_count          = mesh->mNumVertices;
        asset_mesh->vertex_count  = vertex_count;
        asset_mesh->vertices      = malloc_array(Asset_Vertex, asset_mesh->vertex_count);

        for (u32 vertex_idx = 0;
             vertex_idx < vertex_count;
             ++vertex_idx)
        {
            Asset_Vertex *asset_vertex = asset_mesh->vertices + vertex_idx;

            asset_vertex->pos.x = mesh->mVertices[vertex_idx].x;
            asset_vertex->pos.y = mesh->mVertices[vertex_idx].y;
            asset_vertex->pos.z = mesh->mVertices[vertex_idx].z;

            if (mesh->HasNormals()) {
                asset_vertex->normal.x = mesh->mNormals[vertex_idx].x;
                asset_vertex->normal.y = mesh->mNormals[vertex_idx].y;
                asset_vertex->normal.z = mesh->mNormals[vertex_idx].z;
            }

            if (mesh->HasTextureCoords(0)) {
                asset_vertex->uv.x = mesh->mTextureCoords[0][vertex_idx].x;
                asset_vertex->uv.y = mesh->mTextureCoords[0][vertex_idx].y;
            }

            if (mesh->HasVertexColors(0)) {
                asset_vertex->color.r = mesh->mColors[0][vertex_idx].r;
                asset_vertex->color.g = mesh->mColors[0][vertex_idx].g;
                asset_vertex->color.b = mesh->mColors[0][vertex_idx].b;
                asset_vertex->color.a = mesh->mColors[0][vertex_idx].a;
            } else {
                asset_vertex->color = V4(1.0f);
            }

            assert(mesh->HasTangentsAndBitangents());
            asset_vertex->tangent.x = mesh->mTangents[vertex_idx].x;
            asset_vertex->tangent.y = mesh->mTangents[vertex_idx].y;
            asset_vertex->tangent.z = mesh->mTangents[vertex_idx].z;

            for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) {
                asset_vertex->node_ids[i] = -1; // @Spec: Renderer Api must agree it to be speced to -1 too.
                asset_vertex->node_weights[i] = 0;
            }
        }

        assert(mesh->mNumBones <= MAX_BONE_PER_MESH);
        for (u32 bone_idx = 0; bone_idx < mesh->mNumBones; ++bone_idx)
        {
            aiBone *bone = mesh->mBones[bone_idx];
            for (u32 vw_idx = 0;
                 vw_idx < bone->mNumWeights;
                 ++vw_idx)
            {
                aiVertexWeight *vw = bone->mWeights + vw_idx;
                u32 vertex_idx = vw->mVertexId;
                f32 weight = vw->mWeight;
                assert(weight != 0.0f);

                Asset_Vertex *asset_vertex = asset_mesh->vertices + vertex_idx;
                u32 next = get_next_unfilled_bone_index(asset_vertex);
                // assert(next < MAX_BONE_PER_VERTEX);
                s32 bone_id = id_from_name(bone->mName.data, hashmap);
                for (u32 i = 0; i < next; ++i) {
                    if (asset_vertex->node_ids[i] == bone_id) {
                        assert("Duplicate bone!");
                    }
                }
                asset_vertex->node_ids[next] = bone_id;
                asset_vertex->node_weights[next] = weight;
            }
        }

        u32 triangle_count      = mesh->mNumFaces;
        u32 index_count         = triangle_count * 3;
        asset_mesh->index_count = index_count;
        asset_mesh->indices     = malloc_array(u32, asset_mesh->index_count);

        for (u32 triangle_idx = 0;
             triangle_idx < triangle_count;
             ++triangle_idx)
        {
            aiFace *triangle = (mesh->mFaces + triangle_idx);
            assert(triangle->mNumIndices == 3);
            for (u32 i = 0; i < 3; ++i) {
                umm idx_of_idx = (3 * triangle_idx + i);
                asset_mesh->indices[idx_of_idx] = triangle->mIndices[i];
            }
        }


#if 0
        if (mesh->mMaterialIndex >= 0) {
            aiMaterial *material = model->mMaterials[mesh->mMaterialIndex];

            asset_mesh->albedo    = load_texture(material, aiTextureType_BASE_COLOR);
            asset_mesh->normal    = load_texture(material, aiTextureType_NORMALS);
            asset_mesh->metalic   = load_texture(material, aiTextureType_METALNESS);
            asset_mesh->roughness = load_texture(material, aiTextureType_DIFFUSE_ROUGHNESS);
            asset_mesh->emission  = load_texture(material, aiTextureType_EMISSION_COLOR);
            asset_mesh->ao        = load_texture(material, aiTextureType_AMBIENT_OCCLUSION);
        }
#endif
    }
}

internal void
fill_asset_materials(const aiScene *model, Asset_Model *asset_model)
{
    if (model->HasMaterials())
    {
        asset_model->material_count = model->mNumMaterials;
        asset_model->materials = malloc_array(Asset_Material, model->mNumMaterials);

        for (u32 mat_idx = 0;
             mat_idx < model->mNumMaterials;
             ++mat_idx)
        {
            Asset_Material *asset_mat = asset_model->materials + mat_idx;
            aiMaterial *mat = model->mMaterials[mat_idx];

            aiColor3D c;

            mat->Get(AI_MATKEY_COLOR_AMBIENT, c);
            asset_mat->color_ambient = V3(c.r, c.g, c.b);

            mat->Get(AI_MATKEY_COLOR_DIFFUSE, c);
            asset_mat->color_diffuse = V3(c.r, c.g, c.b);

            mat->Get(AI_MATKEY_COLOR_SPECULAR, c);
            asset_mat->color_specular = V3(c.r, c.g, c.b);
        }
    }
}

internal void
write_asset_meshes(FILE *model_out, Asset_Model *asset_model)
{
    fwrite_item(asset_model->mesh_count, model_out);

    for (u32 mesh_idx = 0;
         mesh_idx < asset_model->mesh_count;
         ++mesh_idx)
    {
        Asset_Mesh *asset_mesh = (asset_model->meshes + mesh_idx);

        fwrite_item(asset_mesh->vertex_count, model_out);
        for (u32 vertex_idx = 0;
             vertex_idx < asset_mesh->vertex_count;
             ++vertex_idx)
        {
            Asset_Vertex *vertex = asset_mesh->vertices + vertex_idx;
            fwrite_item(vertex->pos, model_out);
            fwrite_item(vertex->normal, model_out);
            fwrite_item(vertex->uv, model_out);
            fwrite_item(vertex->color, model_out);
            fwrite_item(vertex->tangent, model_out);

            fwrite_array(vertex->node_ids, MAX_BONE_PER_VERTEX, model_out);
            fwrite_array(vertex->node_weights, MAX_BONE_PER_VERTEX, model_out);
        }

        fwrite_item(asset_mesh->index_count, model_out);
        fwrite_array(asset_mesh->indices, asset_mesh->index_count, model_out);
    }
}

internal void
write_asset_materials(FILE *model_out, Asset_Model *asset_model)
{
    fwrite_item(asset_model->material_count, model_out);
    if (asset_model->material_count) {
        fwrite_array(asset_model->materials, asset_model->material_count, model_out);
    }
}

internal void
write_asset_nodes(FILE *model_out, Asset_Model *asset_model)
{
    fwrite_item(asset_model->node_count, model_out);
    if (asset_model->node_count)
    {
        fwrite_item(asset_model->root_bone_node_id, model_out);
        for (u32 node_idx = 0;
             node_idx < asset_model->node_count;
             ++node_idx)
        {
            Asset_Node *asset_node = asset_model->nodes + node_idx;
            fwrite_item(asset_node->id, model_out);
            fwrite_item(asset_node->offset, model_out);
            fwrite_item(asset_node->transform, model_out);
            fwrite_item(asset_node->child_count, model_out);
            fwrite_array(asset_node->child_ids, asset_node->child_count, model_out);
        }
    }
}

internal char *
create_output_animation_filepath(char *in_filepath, char *anim_name)
{
    char *begin = in_filepath;
    char *end   = in_filepath;
    for (char *at = in_filepath; *at; ++at) {
        if (*at == '/') {
            begin = at + 1;
        } else if (*at == '.') {
            end = at;
        }
    }
    int filename_length = end - begin;

    umm directory_length = array_count(PATH_TO_DATA_FROM_BUILD) + string_length(ASSET_ANIMATION_DIRECTORY) + 1;
    char *directory = malloc_array(char, directory_length);
    _snprintf(directory, directory_length, "%s%s", PATH_TO_DATA_FROM_BUILD, ASSET_ANIMATION_DIRECTORY);

    umm filepath_length = directory_length + filename_length + string_length(ASSET_ANIMATION_FILE_FORMAT) + 1;

    char *result = malloc_array(char, filepath_length);
    _snprintf(result, filepath_length, "%s%.*s%s", directory, filename_length, begin, ASSET_ANIMATION_FILE_FORMAT);

    return result;
}

int main(void)
{
    char *input_file_names[] = {
        // "../data/xbot_idle.fbx",
        // "../data/xbot_run.fbx",
        //"../data/skeleton_lord_idle.fbx",
        //"../data/skeleton_lord_run.fbx",
        //"../data/input/model/skeleton_lord_idle.dae",
        //"../data/input/model/skeleton_lord_run.dae",
        //"../data/input/model/skeleton_lord_die.dae",
        //"../data/input/model/skeleton_lord_attack.dae",
        //"../data/input/model/crate.dae",
        //"../data/input/model/sphere.fbx",
        //"../data/input/model/plane.fbx",
        //"../data/input/model/rock.dae",
        "../data/input/model/troll_idle.dae",
        "../data/input/model/troll_walk.dae",
    };

    stbi_set_flip_vertically_on_load(true);

    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    for (u32 file_idx = 0; file_idx < array_count(input_file_names); ++file_idx) 
    {
        char *in_file_name = input_file_names[file_idx];
        const aiScene *model = importer.ReadFile(in_file_name, (aiProcess_Triangulate |
                                                                aiProcess_ImproveCacheLocality |
                                                                aiProcess_CalcTangentSpace |
                                                                aiProcess_OptimizeMeshes |
                                                                aiProcess_RemoveRedundantMaterials |
                                                                aiProcess_LimitBoneWeights |
                                                                aiProcess_GenUVCoords |
                                                                aiProcess_FindDegenerates |
                                                                aiProcess_FindInvalidData |
                                                                aiProcess_FindInstances |
                                                                aiProcess_ValidateDataStructure |
                                                                aiProcess_JoinIdenticalVertices));
        g_node_count = 0;

        if (!model) {
            fprintf(stderr, "[ERROR]: Couldn't load file %s.\n", in_file_name);
            return -1;
        }

        printf("\n[OK]: Load scene '%s'.\n", in_file_name);
        print_scene_abstract(model);

        Hashmap hashmap = {};
        hashmap.length = 500;
        hashmap.next_id = 0;
        hashmap.entries = malloc_array(Hash_Entry, hashmap.length);
        zero_array(hashmap.entries, hashmap.length);

        //
        // Model
        //
        Asset_Model asset_model = {};
        char *out_file_name = create_output_model_filepath(in_file_name);
        FILE *model_out = fopen(out_file_name, "wb");
        if (!model_out) {
            printf("[ERROR]: Couldn't open output file %s\n", out_file_name);
            return -1;
        }

        asset_model.node_count = g_node_count; 
        asset_model.nodes = malloc_array(Asset_Node, asset_model.node_count);
        fill_asset_nodes(model, &asset_model, &hashmap);
        fill_asset_meshes(model, &asset_model, &hashmap);
        fill_asset_materials(model, &asset_model);

        write_asset_meshes(model_out, &asset_model);
        write_asset_materials(model_out, &asset_model);
        write_asset_nodes(model_out, &asset_model);

        // Print status
        printf("[OK]: Written '%s'\n", out_file_name);
        fclose(model_out);


        //
        // Animation
        //
        for (u32 anim_idx = 0;
             anim_idx < model->mNumAnimations;
             ++anim_idx)
        {
            Asset_Animation asset_animation = {};
            aiAnimation *anim = model->mAnimations[anim_idx];

            char *anim_out_file_name = create_output_animation_filepath(in_file_name, anim->mName.data);
            FILE *anim_out = fopen(anim_out_file_name, "wb");

            if (!anim_out) {
                printf("[ERROR]: Couldn't open output file %s\n", anim_out_file_name);
                return -1;
            }

            f32 spt           = 1.0f / (f32)anim->mTicksPerSecond;
            f32 anim_duration = (f32)(anim->mDuration / anim->mTicksPerSecond);
            u32 node_count    = anim->mNumChannels;

            fwrite(anim->mName.data, sizeof(char) * string_length_with_null(anim->mName.data), 1, anim_out);
            fwrite_item(anim_duration, anim_out);
            fwrite_item(node_count, anim_out);
            for (u32 node_idx = 0;
                 node_idx < node_count;
                 ++node_idx)
            {
                aiNodeAnim *node = anim->mChannels[node_idx];

                s32 node_id = id_from_name(node->mNodeName.data, &hashmap);
                u32 translation_count = node->mNumPositionKeys;
                u32 rotation_count = node->mNumRotationKeys;
                u32 scaling_count = node->mNumScalingKeys;

                fwrite_item(node_id, anim_out);

                fwrite_item(translation_count, anim_out);
                fwrite_item(rotation_count, anim_out);
                fwrite_item(scaling_count, anim_out);

                for (u32 idx = 0; idx < translation_count; ++idx) {
                    aiVectorKey key = node->mPositionKeys[idx];
                    f32 dt = (f32)key.mTime * spt;
                    v3 vec = aiv3_to_v3(key.mValue);
                    dt_v3_Pair dt_v3 = dt_v3_Pair{dt, vec};
                    fwrite_item(dt_v3, anim_out);
                }

                for (u32 idx = 0; idx < rotation_count; ++idx) {
                    aiQuatKey key = node->mRotationKeys[idx];
                    f32 dt = (f32)key.mTime * spt;
                    Quaternion q = aiqt_to_qt(key.mValue);
                    dt_qt_Pair dt_qt = dt_qt_Pair{dt, q};
                    fwrite_item(dt_qt, anim_out);
                }

                for (u32 idx = 0; idx < scaling_count; ++idx) {
                    aiVectorKey key = node->mScalingKeys[idx];
                    f32 dt = (f32)key.mTime * spt;
                    v3 vec = aiv3_to_v3(key.mValue);
                    dt_v3_Pair dt_v3 = dt_v3_Pair{dt, vec};
                    fwrite_item(dt_v3, anim_out);
                }
            }

            printf("[OK]: Written '%s'\n", anim_out_file_name);
            fclose(anim_out);
        }
    }

    printf("*** SUCCESSFUL! ***\n");
    return 0;
}
