/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


// @Note: No back-face.
internal void
geogen_plane(Mesh *mesh, Arena *arena, f32 length, u32 subdivision)
{
    if (subdivision > 0) 
    {
        f32 sublength = length / (f32)subdivision;
        u32 patch_count = subdivision*subdivision;
        u32 vertex_count = 4*patch_count;
        mesh->vertex_count = vertex_count;
        mesh->vertices = push_array(arena, Vertex, vertex_count);

        f32 min = -length * 0.5f;

        Vertex *at = mesh->vertices;
        for (u32 z = 0; z < subdivision; ++z) 
        {
            for (u32 x = 0; x < subdivision; ++x) 
            {
                at[0] = Vertex{ v3{min + x*sublength, 0, min + z*sublength}, v3{0,1,0}, v2{x*sublength, 1 - z*sublength}, v4{1,1,1,1}, v3{1,0,0}, {}, {}};
                at[1] = Vertex{ v3{min + x*sublength, 0, min + (z+1)*sublength}, v3{0,1,0}, v2{x*sublength, 1 - (z+1)*sublength}, v4{1,1,1,1}, v3{1,0,0}, {}, {}};
                at[2] = Vertex{ v3{min + (x+1)*sublength, 0, min + z*sublength}, v3{0,1,0}, v2{(x+1)*sublength, 1 - z*sublength}, v4{1,1,1,1}, v3{1,0,0}, {}, {}};
                at[3] = Vertex{ v3{min + (x+1)*sublength, 0, min + (z+1)*sublength}, v3{0,1,0}, v2{(x+1)*sublength, 1 - (z+1)*sublength}, v4{1,1,1,1}, v3{1,0,0}, {}, {}};
                at += 4;
            }
        }

    } 
    else 
    {
        INVALID_CODE_PATH;
    }
}

// @Note: Do not use it except for quad tessellated terrain.
internal void
geogen_backfaced_cube(Mesh *mesh, Arena *arena, f32 scale)
{
    Vertex vertices[] = 
    {
        Vertex{v3{ 1, 1, 1}*scale, v3{ 1, 0, 0}, v2{0,1}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1,-1, 1}*scale, v3{ 1, 0, 0}, v2{0,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1,-1,-1}*scale, v3{ 1, 0, 0}, v2{1,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1, 1,-1}*scale, v3{ 1, 0, 0}, v2{1,1}, RGBA_WHITE, {}, {}},

        Vertex{v3{-1,-1,-1}*scale, v3{-1, 0, 0}, v2{0,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1,-1, 1}*scale, v3{-1, 0, 0}, v2{1,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1, 1,-1}*scale, v3{-1, 0, 0}, v2{0,1}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1, 1, 1}*scale, v3{-1, 0, 0}, v2{1,1}, RGBA_WHITE, {}, {}},

        Vertex{v3{-1, 1, 1}*scale, v3{ 0, 1, 0}, v2{0,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1, 1, 1}*scale, v3{ 0, 1, 0}, v2{1,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1, 1,-1}*scale, v3{ 0, 1, 0}, v2{0,1}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1, 1,-1}*scale, v3{ 0, 1, 0}, v2{1,1}, RGBA_WHITE, {}, {}},

        Vertex{v3{ 1,-1, 1}*scale, v3{ 0, 1, 0}, v2{0,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1,-1, 1}*scale, v3{ 0, 1, 0}, v2{1,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1,-1,-1}*scale, v3{ 0, 1, 0}, v2{0,1}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1,-1,-1}*scale, v3{ 0, 1, 0}, v2{1,1}, RGBA_WHITE, {}, {}},

        Vertex{v3{ 1,-1,-1}*scale, v3{ 0, 1, 0}, v2{0,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1,-1,-1}*scale, v3{ 0, 1, 0}, v2{1,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1, 1,-1}*scale, v3{ 0, 1, 0}, v2{0,1}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1, 1,-1}*scale, v3{ 0, 1, 0}, v2{1,1}, RGBA_WHITE, {}, {}},

        Vertex{v3{-1,-1, 1}*scale, v3{ 0, 1, 0}, v2{0,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1,-1, 1}*scale, v3{ 0, 1, 0}, v2{1,0}, RGBA_WHITE, {}, {}},
        Vertex{v3{-1, 1, 1}*scale, v3{ 0, 1, 0}, v2{0,1}, RGBA_WHITE, {}, {}},
        Vertex{v3{ 1, 1, 1}*scale, v3{ 0, 1, 0}, v2{1,1}, RGBA_WHITE, {}, {}},
    };
    u32 indices[] = {
        0, 3, 1, 1, 3, 2,
        4, 6, 7, 4, 7, 5,
        8,10,11, 8,11, 9,
        12,14,15,12,15,13,
        16,18,19,16,19,17,
        20,22,23,20,23,21
    };

    u32 vertex_count = array_count(vertices);
    mesh->vertex_count = vertex_count;
    mesh->vertices = push_array(arena, Vertex, vertex_count);
    memory_copy(mesh->vertices, vertices, sizeof(*vertices)*vertex_count);

    u32 index_count = array_count(indices);
    mesh->index_count = index_count;
    mesh->indices  = push_array(arena, u32, index_count);
    memory_copy(mesh->indices, indices, sizeof(*indices)*index_count);
}
