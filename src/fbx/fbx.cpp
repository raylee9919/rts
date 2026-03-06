// Copyright Seong Woo Lee. All Rights Reserved.

// C++
//
#include <vector>
#include <queue>
#include <string>
#include <set>
#include <unordered_map>

#include "third_party/ufbx/ufbx.h"
#include "third_party/ufbx/ufbx.c"

// .h
//
#include "third_party/xxhash3/xxhash.c"
#include "base/rts_base_inc.h"
#include "os/rts_os.h"
#include "asset.h"

// .cpp
//
#include "base/rts_base_inc.cpp"
#include "os/rts_os.cpp"


#include "fbx.h"


struct State {
    Arena *arena;

    Arena *scene_arena;
    ufbx_scene *scene;

    Asset_Mesh *meshes;
    u32 num_meshes;
};

void fbx_print_nodes(ufbx_node *node, int depth = 0)
{
    ufbx_string name = node->name;
    printf("%*s", depth*2, " ");
    printf("%.*s\n", (int)name.length, name.data);

    ufbx_node_list children = node->children;
    for (int i = 0; i < (int)children.count; ++i) {
        ufbx_node *child = children.data[i];
        fbx_print_nodes(child, depth + 1);
    }
}

void fbx_get_meshes_recursively(State *state, ufbx_node *node)
{
    Arena *arena = state->scene_arena;

    if (node->attrib_type == UFBX_ELEMENT_MESH) {
        ufbx_mesh *fbx_mesh = node->mesh;

        Asset_Mesh mesh = {};

        // Get name.
        mesh.length = (u32)node->name.length;
        mesh.name   = push_array(arena, u8, mesh.length + 1);
        memcpy(mesh.name, node->name.data, sizeof(u8)*mesh.length);
        mesh.name[mesh.length] = 0;

        // Push the mesh.
        state->meshes[state->num_meshes++] = mesh;
    }

    // Traverse down.
    ufbx_node_list children = node->children;
    for (int i = 0; i < (int)children.count; ++i) {
        fbx_get_meshes_recursively(state, children.data[i]);
    }
}

void fbx_get_meshes(State *state)
{
    u32 num_meshes = (u32)state->scene->meshes.count;
    state->meshes = push_array(state->scene_arena, Asset_Mesh, num_meshes);

    fbx_get_meshes_recursively(state, state->scene->root_node);

    // Does the actual mesh count match the expected count?
    assert(state->num_meshes == num_meshes);
}

int main()
{
    os_init();
    thread_init();

    State *state;
    {
        Arena *arena = arena_alloc();
        state = push_struct(arena, State);
        state->arena = arena;
        state->scene_arena = arena_alloc();
    }


    {
        Utf8 in_file  = utf8lit("C:/dev/rts/data/input/knight.fbx");
        Utf8 out_file = utf8lit("C:/dev/rts/data/TEST.triangle_mesh");

        ufbx_load_opts opts = {};
        ufbx_error error;

        ufbx_scene *scene = ufbx_load_file((const char *)in_file.str, &opts, &error);

        if (!scene) {
            printf("ERROR: Failed to load file '%.*s'.\n", (int)in_file.len, (const char *)in_file.str);
            return -1;
        }

        state->scene = scene;

        //fbx_print_nodes(scene->root_node);
        fbx_get_meshes(state);

        // Write asset.
        //
        {
            FILE *f = fopen((const char *)out_file.str, "wb");
            if (!f) {
                printf("ERROR: Failed to open file '%.*s'.\n", (int)out_file.len, (const char *)out_file.str);
                return -1;
            }

            fprintf(f, "%u\n\n", state->num_meshes);

            for (u32 mi = 0; mi < state->num_meshes; ++mi) {
                auto *mesh = &state->meshes[mi];

                fprintf(f, "%u %.*s\n", mesh->length, (int)mesh->length, (const char *)mesh->name);
            }

            fclose(f);
        }

        arena_clear(state->scene_arena);
        ufbx_free_scene(scene);
    }


    printf("***SUCCESSFUL!***\n");
    return 0;
}

v2 v2_from_ufbx_vec2(ufbx_vec2 v)
{
    return v2(v.x, v.y);
}

v3 v3_from_ufbx_vec3(ufbx_vec3 v)
{
    return v3(v.x, v.y, v.z);
}
