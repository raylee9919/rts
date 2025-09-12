#ifndef RTS_DELAUNAY_H
#define RTS_DELAUNAY_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */


struct Nav_Constrain 
{
    int (*edges)[2];
    u32 edge_count;
    u32 edge_size;
};

struct Cdt_Result 
{
    int numtri;
    int (*tri)[3];
    int (*adj)[3];
    s32 *trespassable;
};

struct Nav_Vertex 
{
    int idx; // if it is < 0, it isn't in the array.
    v3 position;
};

struct Nav_Portal 
{
    Nav_Vertex left;
    Nav_Vertex right;
};

struct Navmesh 
{
    Arena *arena;
    Cdt_Result cdt;

    Vertex *vertices;
    u32 vertex_size;
    u32 vertex_count;

    Nav_Constrain *constrains;
    u32 constrain_size;
    u32 constrain_count;

    b32 is_constrain;
    u32 first_vertex;
};


internal u32 cdt_bin_partition(int *VIDX, u32 lo, u32 hi, int *BIN);
internal void cdt_bin_quicksort(int *VIDX, u32 lo, u32 hi, int *BIN);
internal b32 cdt_point_in_triangle(v2 p, v2 a, v2 b, v2 c);
internal int cdt_edge(int (*adj)[3], int L, int K);
internal b32 point_on_left(v2 p, v2 a, v2 b, b32 colinear);
internal b32 point_on_right(v2 p, v2 a, v2 b, b32 colinear);
internal b32 cdt_intersect(v2 a1, v2 a2, v2 b1, v2 b2);
internal b32 is_convex(v2 a, v2 b, v2 c, v2 d);
internal b32 cdt_bad(f32 xp, f32 x1, f32 x2, f32 x3, f32 yp, f32 y1, f32 y2, f32 y3);
internal Cdt_Result delaunay_triangulate(Vertex *vertices, u32 vertexcount, Navmesh *navmesh);
internal void begin_constrain(Navmesh *nv);
internal void end_constrain(Navmesh *nv);
internal void push_vertex(Navmesh *nv, v3 v);


#endif // RTS_DELAUNAY_H
