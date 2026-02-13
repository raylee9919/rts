// Copyright Seong Woo Lee. All Rights Reserved.

// Assimp includes.
//
#include "vendor/assimp/Importer.hpp"
#include "vendor/assimp/scene.h"
#include "vendor/assimp/postprocess.h"

// [.h]
//
#include "third_party/xxhash3/xxhash.c"
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "rts_font.h"
#include "asset.h"

#define STBI_ASSERT(x)
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

#include "rts_assimp.h"

// [.cpp]
//
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"

#include <vector>
#include <queue>
#include <string>
#include <set>
#include <unordered_map>



struct Asset_Joint {
    u16 length;
    u8 *name;

    s32 parent;
    m4x4 local_transform;
    m4x4 inverse_rest_pose;
};

struct Asset_Skeleton {
    u32 num;
    Asset_Joint *joints;
};

struct State {
    aiScene *scene;
    u32 num_nodes;

    bool has_skeleton;

    std::unordered_map <std::string, s32> bone_map;
};


// Custom hash function for aiString. Fucking C++ idiocracy :(
template <>
struct std::hash<aiString> {
    std::size_t operator()(const aiString& str) const {
        return XXH3_64bits_withSeed(str.data, strlen(str.data), 0);
    }
};


//
// Skeleton
//
static void fill_table_recursively(std::unordered_map<aiString, aiNode*>& table, aiNode *node) {
    if (table.find(node->mName) == table.end()) {
        table[node->mName] = node;
    }

    for (u32 i = 0; i < node->mNumChildren; ++i) {
        aiNode *child = node->mChildren[i];
        fill_table_recursively(table, child);
    }
}

static std::vector <Asset_Joint> make_joint_array(State *state) {
    using namespace std;

    aiScene *scene = state->scene;
    aiNode *root = scene->mRootNode;


    // Build a bone's name to node ptr map.
    // This'll not contain the root bone!
    //
    unordered_map <aiString, pair<aiNode*, aiBone*>> bone_map;

    for (u32 mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh *mesh = scene->mMeshes[mi];
        for (u32 bi = 0; bi < mesh->mNumBones; ++bi) {
            aiBone *bone   = mesh->mBones[bi];
            aiString name  = bone->mName;
            aiNode* node   = root->FindNode(name);
            bone_map[name] = {node, bone};
        }
    }



    // Find the root bone by iterating over every node in the scene.
    // If the node is in the bone-map, and its parent isn't in the bone-map, 
    // that parent must be the root node.
    //
    aiNode *root_bone = nullptr;
    {
        queue <aiNode*> q;
        q.push(root);

        while (!q.empty()) {
            aiNode *node = q.front();
            q.pop();

            if (bone_map.find(node->mName) != bone_map.end()) {
                if (bone_map.find(node->mParent->mName) == bone_map.end()) {
                    root_bone = node->mParent;
                    break;
                }
            }

            for (u32 i = 0; i < node->mNumChildren; ++i) {
                q.push(node->mChildren[i]);
            }
        }

    }
    assert( root_bone );



    struct Node_Parent {
        aiNode *node;
        s32 parent;
    };

    vector <Asset_Joint> joints;
    {
        s32 idx = 0;

        queue <Node_Parent> q;
        q.push({root_bone, -1});

        while (!q.empty()) {
            auto item = q.front();
            q.pop();

            aiNode *node = item.node;
            s32 parent   = item.parent;

            for (u32 i = 0; i < node->mNumChildren; ++i) {
                // We only care about bone nodes. The root bone node we retrieved may represent more than just a bone, 
                // and its children can include non-bone nodes, which are not relevant here.
                //
                // BUT ONLY FOR THE ROOT NODE!!
                //
                aiNode *child = node->mChildren[i];

                if ((idx == 0) && (bone_map.find(child->mName) == bone_map.end())) {
                    continue;
                }

                q.push({child, idx});
            }


            joints.push_back({});
            auto* joint = &joints[joints.size() - 1];
            {
                aiString name = node->mName; 
                u16 len = (u16)strlen(name.data);

                joint->length = len;
                joint->name   = new u8[len];
                memcpy(joint->name, name.data, len * sizeof(joint->name[0]));

                joint->parent = parent;
                joint->local_transform = to_m4x4(node->mTransformation);

                // @Todo: mOffsetMatrix is fucking insane. How can I derive this motherfucker?
                // It's so annoying. Before I figure that out, it is fragile.
                //
                if (bone_map.find(name) != bone_map.end()) {
                    joint->inverse_rest_pose = to_m4x4(bone_map[name].second->mOffsetMatrix);
                } else {
                    joint->inverse_rest_pose = inverse(to_m4x4(node->mTransformation));
                }
            }

            idx++;
        }
    }
    // MAX_BONE_PER_MESH is a wrong name.
    assert( joints.size() <= MAX_BONE_PER_MESH );

    return joints;
}

static std::unordered_map<std::string, s32>
make_bone_name_to_index_map(std::vector<Asset_Joint>& joints) {
    using namespace std;

    unordered_map<string, s32> result;

    u32 idx = 0;
    for (auto j : joints) {
        result[string((char *)j.name, (size_t)j.length)] = idx++;
    }

    return result;
}

//
// Model
//
static void make_model(State *state, Asset_Model *model_out) {
    aiScene *scene = state->scene;

    u32 num_meshes = scene->mNumMeshes;

    model_out->meshes = new Asset_Mesh[num_meshes];
    model_out->mesh_count = num_meshes;

    for (u32 mi = 0; mi < num_meshes; ++mi) {
        Asset_Mesh *mesh_out = &model_out->meshes[mi];
        aiMesh *mesh = scene->mMeshes[mi];

        u32 num_vert    = mesh->mNumVertices;
        u32 num_tri     = mesh->mNumFaces;
        u32 num_indices = num_tri * 3;

        mesh_out->length = (u32)strlen(mesh->mName.data);
        mesh_out->name = new u8[mesh_out->length];
        memcpy(mesh_out->name, mesh->mName.data, mesh_out->length);

        mesh_out->vertices = new Asset_Vertex[num_vert];
        mesh_out->vertex_count = num_vert;


        for (u32 vi = 0; vi < num_vert; ++vi) {
            auto vert_out = &mesh_out->vertices[vi];
            auto vert     = mesh->mVertices[vi];

            // Write positions.
            vert_out->pos.x = vert.x;
            vert_out->pos.y = vert.y;
            vert_out->pos.z = vert.z;

            // Write normals.
            if (mesh->HasNormals()) {
                vert_out->normal.x = mesh->mNormals[vi].x;
                vert_out->normal.y = mesh->mNormals[vi].y;
                vert_out->normal.z = mesh->mNormals[vi].z;
            }

            // Write UVs.
            if (mesh->HasTextureCoords(0)) {
                vert_out->uv.x = mesh->mTextureCoords[0][vi].x;
                vert_out->uv.y = mesh->mTextureCoords[0][vi].y;
            }

            // Write vert colors.
            if (mesh->HasVertexColors(0)) {
                vert_out->color.r = mesh->mColors[0][vi].r;
                vert_out->color.g = mesh->mColors[0][vi].g;
                vert_out->color.b = mesh->mColors[0][vi].b;
                vert_out->color.a = mesh->mColors[0][vi].a;
            } else {
                vert_out->color = v4{1,1,1,1};
            }

            // Write tangents.
            assert( mesh->HasTangentsAndBitangents() ); // We forced the API to create one.
            vert_out->tangent.x = mesh->mTangents[vi].x;
            vert_out->tangent.y = mesh->mTangents[vi].y;
            vert_out->tangent.z = mesh->mTangents[vi].z;

            // Initialize joint data.
            for (u32 i = 0; i < MAX_BONE_PER_VERTEX; ++i) {
                vert_out->node_ids[i]     = -1; // @Robustness: Hard coded -1.
                vert_out->node_weights[i] = 0.f;
            }
        }

        for (u32 bi = 0; bi < mesh->mNumBones; ++bi) {
            aiBone *bone = mesh->mBones[bi];
            std::string name(bone->mName.data);
            s32 bone_idx = state->bone_map[name];

            for (u32 i = 0; i < bone->mNumWeights; ++i) {
                aiVertexWeight *vw = &bone->mWeights[i];
                u32 vert_idx = vw->mVertexId;
                f32 weight   = vw->mWeight;

                Asset_Vertex *vert_out = &mesh_out->vertices[vert_idx];

                // @Todo: Sort the weights and use the topmost 4 of them..
                bool duplicated = false;
                u32 next = 0xBEEF;
                for (u32 j = 0; j < MAX_BONE_PER_VERTEX; ++j) {
                    if (vert_out->node_ids[j] == -1) {
                        next = j;
                        break;
                    } else if (vert_out->node_ids[j] == bone_idx) {
                        duplicated = true;
                        break;
                    }
                }

                if (!duplicated) {
                    assert( next != 0xBEEF );
                    vert_out->node_ids[next]     = bone_idx;
                    vert_out->node_weights[next] = weight;
                }
            }
        }



        // Write indices.
        mesh_out->indices     = new u32[num_indices];
        mesh_out->index_count = num_indices;
        mesh_out->indices     = new u32[num_indices];

        for (u32 ti = 0; ti < num_tri; ++ti) {
            aiFace *tri = &mesh->mFaces[ti];

            assert( tri->mNumIndices == 3 ); // We forced triangulation.

            for (u32 i = 0; i < 3; ++i) {
                mesh_out->indices[ti * 3 + i] = tri->mIndices[i];
            }
        }
    }
}

//
// Animations
//
int cmp_bone_id(const void *a, const void *b) {
    Asset_Animation_Node *l = (Asset_Animation_Node *)a;
    Asset_Animation_Node *r = (Asset_Animation_Node *)b;

    if (l->id < r->id) return -1;
    if (l->id > r->id) return  1;
    return 0;
}

static void make_animation(State *state, Asset_Animation *anim_out) {
    aiScene *scene = state->scene;

    // If we want more, gonna revamp the function.
    assert( scene->mNumAnimations == 1 );

    for (u32 ai = 0; ai < scene->mNumAnimations; ++ai) {
        aiAnimation *anim = scene->mAnimations[ai];

        // f32 anim_duration = (f32)(anim->mDuration / anim->mTicksPerSecond);
        f32 fps = (f32)anim->mTicksPerSecond;
        u32 num_nodes = anim->mNumChannels;

        anim_out->length = (u32)strlen(anim->mName.data);
        anim_out->name = new u8[anim_out->length];
        memcpy(anim_out->name, anim->mName.data, anim_out->length);
        anim_out->fps = fps;
        anim_out->num_nodes = num_nodes;
        anim_out->nodes = new Asset_Animation_Node[num_nodes];

        u32 num = 0;
        for (u32 ni = 0; ni < num_nodes; ++ni) {
            aiNodeAnim *node = anim->mChannels[ni];
            u32 num_translations = node->mNumPositionKeys;
            u32 num_rotations    = node->mNumRotationKeys;
            u32 num_scales       = node->mNumScalingKeys;
            num = max(num, max(max(num_translations, num_rotations), num_scales));
        }
        assert( num != 0);
        anim_out->num_samples = num;

        for (u32 ni = 0; ni < num_nodes; ++ni) {

            aiNodeAnim *node = anim->mChannels[ni];
            auto* node_out = &anim_out->nodes[ni];

            s32 bone_id = state->bone_map[std::string(node->mNodeName.data)];
            node_out->id = bone_id;

            node_out->translations = new v3[num];
            node_out->rotations    = new Quaternion[num];
            node_out->scales       = new v3[num];

            u32 num_translations = node->mNumPositionKeys;
            u32 num_rotations    = node->mNumRotationKeys;
            u32 num_scales       = node->mNumScalingKeys;

            for (u32 i = 0; i < num; ++i) {
                u32 idx = min(i, num_translations - 1);
                aiVectorKey key = node->mPositionKeys[idx];
                node_out->translations[i] = to_v3(key.mValue);
            }

            for (u32 i = 0; i < num; ++i) {
                u32 idx = min(i, num_rotations - 1);
                aiQuatKey key = node->mRotationKeys[idx];
                node_out->rotations[i] = to_quaternion(key.mValue);
            }

            for (u32 i = 0; i < num; ++i) {
                u32 idx = min(i, num_scales - 1);
                aiVectorKey key = node->mScalingKeys[idx];
                node_out->scales[i] = to_v3(key.mValue);
            }


        }
    }

    qsort(anim_out->nodes, anim_out->num_nodes, sizeof(anim_out->nodes[0]), cmp_bone_id);
}

//
// Scene
//
static u32 get_node_count(aiNode *node) {
    u32 num = 1;
    for (u32 i = 0; i < node->mNumChildren; ++i) {
        num += get_node_count(node->mChildren[i]);
    }
    return num;
}

static void print_scene_hierarchy_recursively(aiNode *node, int depth = 0) {
    for (int i = 0; i < depth; ++i) printf("  ");
    printf("%s\n", node->mName.data);

    m4x4 transform = to_m4x4(node->mTransformation);
    for (s32 r = 0; r < 4; ++r) {
        printf("%*s", depth << 1, "");
        for (s32 c = 0; c < 4; ++c) {
            printf("%.6ff, ", transform.e[r][c]);
        }
        printf("\n");
    }

    for (u32 i = 0; i < node->mNumChildren; ++i) {
        aiNode *child = node->mChildren[i];
        print_scene_hierarchy_recursively(child, depth + 1);
    }
}

static bool scene_has_skeleton(aiScene *scene) {
    for (u32 mi = 0; mi < scene->mNumMeshes; ++mi) {
        aiMesh *mesh = scene->mMeshes[mi];
        if (mesh->HasBones()) {
            return true;
        }
    }
    return false;
}

int main(void)
{
    // Init codebase.
    //
    os_init();
    thread_init();


    State *state = new State;

    // @Temporary
    // @Temporary
    // @Temporary
    m4x4 root = {
        0.010000f, 0.000000f, 0.000000f, 0.000000f,
        0.000000f, 0.010000f, 0.000000f, 0.000000f,
        0.000000f, 0.000000f, 0.010000f, 0.000000f,
        0.000000f, 0.000000f, 0.000000f, 1.000000f,
    };
    m4x4 hip = {
        -0.581853f, 0.800314f, 0.144723f, -0.694497f,
        0.018189f, 0.190707f, -0.981478f, 83.361526f,
        -0.813091f, -0.568444f, -0.125520f, 0.886050f,
        0.000000f, 0.000000f, 0.000000f, 1.000000f,
    };
    m4x4 scabbard = {
        -0.078485f, -0.614300f, 0.785159f, -16.189623f,
        0.649227f, 0.566186f, 0.507875f, -13.446239f,
        -0.756534f, 0.549607f, 0.354384f, 2.368096f,
        0.000000f, 0.000000f, 0.000000f, 1.000000f,
    };
    m4x4 scabbard2 = {
        1.000000f, 0.000000f, 0.000000f, 0.000000f,
        0.000000f, 1.000000f, 0.000000f, 0.000000f,
        0.000000f, 0.000000f, 1.000000f, 0.000000f,
        0.000000f, 0.000000f, 0.000000f, 1.000000f,
    };

    m4x4 res1 = hip * scabbard * scabbard2;
    m4x4 inv1 = inverse(res1);
    // @Temporary
    // @Temporary
    // @Temporary

    char *input_file_names[] = {
        //"../data/input/model/skeleton_lord_rest_pose.dae",
        "../data/input/model/skeleton_lord_idle.dae",
        //"../data/input/model/skeleton_lord_run.dae",
        //"../data/input/model/skeleton_lord_die.dae",
        //"../data/input/model/skeleton_lord_attack.dae",
        //"../data/input/model/sword.dae",
        //"../data/input/model/plane.fbx",
        //"../data/input/model/castle.fbx",
    };

    stbi_set_flip_vertically_on_load(true);

    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    for (u32 file_idx = 0; file_idx < array_count(input_file_names); ++file_idx) {

        char *in_file_name = input_file_names[file_idx];
        state->scene = (aiScene *)importer.ReadFile(in_file_name, (aiProcess_Triangulate |
                                                                   aiProcess_ImproveCacheLocality |
                                                                   aiProcess_CalcTangentSpace |
                                                                   aiProcess_GlobalScale |
                                                                   aiProcess_OptimizeMeshes |
                                                                   aiProcess_OptimizeGraph |
                                                                   aiProcess_RemoveRedundantMaterials |
                                                                   aiProcess_LimitBoneWeights |
                                                                   aiProcess_GenUVCoords |
                                                                   aiProcess_FindDegenerates |
                                                                   aiProcess_FindInvalidData |
                                                                   aiProcess_FindInstances |
                                                                   aiProcess_ValidateDataStructure |
                                                                   aiProcess_JoinIdenticalVertices));

        // Load scene.
        //
        {
            if (!state->scene) {
                fprintf(stderr, "[ERROR]: Couldn't load file %s.\n", in_file_name);
                return -1;
            }

            printf("\nLoaded scene '%s'.\n", in_file_name);

            state->num_nodes = get_node_count(state->scene->mRootNode);
            printf("# of aiNode: %d\n", state->num_nodes);

            //print_scene_hierarchy_recursively(state->scene->mRootNode);
        }


        // Make skeleton asset if there's one.
        //
        state->has_skeleton = scene_has_skeleton(state->scene);
        if (state->has_skeleton) {
            using namespace std;

            // Index = ID.
            vector <Asset_Joint> joints = make_joint_array(state);

            // Since assimp vertices reference bones by name, we build a mapping from bone names to joint indices.
            state->bone_map = make_bone_name_to_index_map(joints);
            printf("# of bones: %u\n", (u32)joints.size());



            // @Temporary
            FILE *f = fopen("C:/dev/rts/data/skeleton_lord.skeleton", "wb");
            assert(f);
            {
                fprintf(f, "%u\n", (u32)joints.size());
                fprintf(f, "\n");

                int idx = 0;
                for (auto joint : joints) {
                    fprintf(f, "%u %.*s\n", joint.length, joint.length, joint.name);
                    fprintf(f, "%d\n", joint.parent);
                    m4x4 m = joint.local_transform;
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", m._11, m._12, m._13, m._14);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", m._21, m._22, m._23, m._24);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", m._31, m._32, m._33, m._34);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", m._41, m._42, m._43, m._44);
                    fprintf(f, "\n");
                    m4x4 inv = joint.inverse_rest_pose;
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._11, inv._12, inv._13, inv._14);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._21, inv._22, inv._23, inv._24);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._31, inv._32, inv._33, inv._34);
                    fprintf(f, "%.6f %.6f %.6f %.6f\n", inv._41, inv._42, inv._43, inv._44);
                    fprintf(f, "\n");
                }
            }
            fclose(f);
        }




        // Make model asset.
        //
        {
            Asset_Model *model = new Asset_Model;
            make_model(state, model);

            // @Temporary
            FILE *f = fopen("C:/dev/rts/data/skeleton_lord.triangle_mesh", "wb");
            assert(f);
            {
                fprintf(f, "%u\n\n", model->mesh_count);

                for (u32 mi = 0; mi < model->mesh_count; ++mi) {
                    Asset_Mesh *mesh = &model->meshes[mi];

                    fprintf(f, "%u %.*s\n", mesh->length, mesh->length, mesh->name);

                    fprintf(f, "%u\n", mesh->vertex_count);
                    for (u32 vi = 0; vi < mesh->vertex_count; ++vi) {
                        Asset_Vertex *vert = &mesh->vertices[vi];
                        fprintf(f, "%.6f %.6f %.6f\n", vert->pos.x, vert->pos.y, vert->pos.z);
                        fprintf(f, "%.6f %.6f %.6f\n", vert->normal.x, vert->normal.y, vert->normal.z);
                        fprintf(f, "%.6f %.6f\n", vert->uv.x, vert->uv.y);
                        fprintf(f, "%.6f %.6f %.6f %.6f\n", vert->color.r, vert->color.g, vert->color.b, vert->color.a);
                        fprintf(f, "%.6f %.6f %.6f\n", vert->tangent.x, vert->tangent.y, vert->tangent.z);
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

                        if ((i+1) % 10 == 0) {
                            fprintf(f, "\n");
                        }
                    }
                    fprintf(f, "\n");
                }
            }
            fclose(f);
        }


        // Make animation asset if there's any.
        //
        if (state->scene->HasAnimations()) {
            Asset_Animation *anim = new Asset_Animation;
            make_animation(state, anim);

            // @Temporary
            FILE *f = fopen("C:/dev/rts/data/skeleton_lord.keyframed_animation", "wb");
            assert(f);
            {
                fprintf(f, "%u %.*s\n", anim->length, anim->length, anim->name);
                fprintf(f, "%.6f\n", anim->fps);
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
    }




#if 0
        Asset_Model asset_model = {};
        {
            asset_model.node_count = get_unique_joint_count(scene); 
            asset_model.nodes      = new Asset_Node[asset_model.node_count];

            printf("joint count: %u\n", asset_model.node_count);
        }


        fill_asset_nodes(scene, &asset_model, &hashmap);
        fill_asset_meshes(scene, &asset_model, &hashmap);
        fill_asset_materials(scene, &asset_model);

        write_asset_meshes(model_out, &asset_model);
        write_asset_materials(model_out, &asset_model);
        write_asset_nodes(model_out, &asset_model);

        // Print status
        printf("[OK]: Written '%s'\n", out_file_name);
        fclose(model_out);


        //
        // Animation
        //
        for (u32 anim_idx = 0; anim_idx < scene->mNumAnimations; ++anim_idx) {

            Asset_Animation asset_animation = {};
            aiAnimation *anim = scene->mAnimations[anim_idx];

            char *anim_out_file_name = create_output_animation_filepath(in_file_name, anim->mName.data);
            FILE *anim_out = fopen(anim_out_file_name, "wb");

            if (!anim_out) {
                printf("[ERROR]: Couldn't open output file %s\n", anim_out_file_name);
                return -1;
            }

            f32 spt           = 1.0f / (f32)anim->mTicksPerSecond;
            f32 anim_duration = (f32)(anim->mDuration / anim->mTicksPerSecond);
            u32 node_count    = anim->mNumChannels;

            fwrite(anim->mName.data, sizeof(char) * strlen(anim->mName.data) + 1, 1, anim_out);
            fwrite_item(anim_duration, anim_out);
            fwrite_item(node_count, anim_out);

            for (u32 node_idx = 0; node_idx < node_count; ++node_idx) {

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
                    v3 vec = to_v3(key.mValue);
                    dt_v3_Pair dt_v3 = dt_v3_Pair{dt, vec};
                    fwrite_item(dt_v3, anim_out);
                }

                for (u32 idx = 0; idx < rotation_count; ++idx) {
                    aiQuatKey key = node->mRotationKeys[idx];
                    f32 dt = (f32)key.mTime * spt;
                    Quaternion q = to_quaternion(key.mValue);
                    dt_qt_Pair dt_qt = dt_qt_Pair{dt, q};
                    fwrite_item(dt_qt, anim_out);
                }

                for (u32 idx = 0; idx < scaling_count; ++idx) {
                    aiVectorKey key = node->mScalingKeys[idx];
                    f32 dt = (f32)key.mTime * spt;
                    v3 vec = to_v3(key.mValue);
                    dt_v3_Pair dt_v3 = dt_v3_Pair{dt, vec};
                    fwrite_item(dt_v3, anim_out);
                }
            }

            printf("[OK]: Written '%s'\n", anim_out_file_name);
            fclose(anim_out);
        }
    }

#endif

    printf("*** SUCCESSFUL! ***\n");
    return 0;
}

//
// Conversion
//
v3 to_v3(aiVector3D ai) {
    v3 v;
    v.x = ai.x;
    v.y = ai.y;
    v.z = ai.z;
    return v;
}

Quaternion to_quaternion(aiQuaternion ai) 
{
    Quaternion q;
    q.w = ai.w;
    q.x = ai.x;
    q.y = ai.y;
    q.z = ai.z;
    return q;
}

m4x4 to_m4x4(aiMatrix4x4 ai_mat) 
{
    m4x4 mat;
    mat._11 = ai_mat.a1;
    mat._12 = ai_mat.a2;
    mat._13 = ai_mat.a3;
    mat._14 = ai_mat.a4;

    mat._21 = ai_mat.b1;
    mat._22 = ai_mat.b2;
    mat._23 = ai_mat.b3;
    mat._24 = ai_mat.b4;

    mat._31 = ai_mat.c1;
    mat._32 = ai_mat.c2;
    mat._33 = ai_mat.c3;
    mat._34 = ai_mat.c4;

    mat._41 = ai_mat.d1;
    mat._42 = ai_mat.d2;
    mat._43 = ai_mat.d3;
    mat._44 = ai_mat.d4;

    return mat;
}
