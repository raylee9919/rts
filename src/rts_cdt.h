// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once


typedef u64 cdt_id;

typedef struct cdt_vertex cdt_vertex;
typedef struct cdt_quad_edge cdt_quad_edge;
typedef struct cdt_edge cdt_edge;

typedef struct {
    cdt_vertex **data;
    int num;
    int cap;
} cdt_vertex_array;

typedef struct {
    cdt_id *data;
    int num;
    int cap;
} cdt_id_array;

typedef struct {
    cdt_quad_edge **data;
    int num;
    int cap;
} cdt_quad_edge_array;

typedef struct {
    cdt_edge **data;
    int num;
    int cap;
} cdt_edge_array;

typedef struct {
    cdt_quad_edge **data;
    int front;
    int back;
    int cap;
} cdt_queue;

struct cdt_vertex {
    v2 pos;
    cdt_quad_edge_array edges; // A list of quad-edges originating from the vertex.
};

struct cdt_quad_edge {
    cdt_vertex    *org;
    cdt_quad_edge *onext_ptr;
    uint8_t        idx;       // [0,3]
    uint8_t        visited;
};

struct cdt_edge {
    cdt_quad_edge e[4];
    cdt_id_array  ids;
};

typedef struct {
    cdt_vertex_array vertices;
    cdt_edge_array   edges;
} Cdt_Context;

typedef struct {
    int is_exact_vertex;
    int is_on_edge;
    cdt_vertex *vertex;
    cdt_quad_edge *edge;
} cdt_locate_result;

typedef struct {
    cdt_vertex *vert;
    f32 dx;
    f32 dy;
} cdt_vertex_sort_struct;

typedef struct cdt_index_node cdt_index_node;
struct cdt_index_node {
    int idx;
    cdt_index_node *next;
};

typedef struct {
    cdt_quad_edge *edges[3];
    f32 x[3];
    f32 y[3];
} cdt_triangle;

typedef struct {
    cdt_triangle triangles[3];
} cdt_triangles;


static void           cdt_init(Cdt_Context *ctx, f32 x1, f32 y1, f32 x2, f32 y2, f32 x3, f32 y3);

static void           cdt_insert(Cdt_Context *ctx, cdt_id id, f32 x1, f32 y1, f32 x2, f32 y2);
static void           cdt_remove(Cdt_Context *ctx, cdt_id id);

static int            cdt_is_constrained(cdt_edge *edge);
static int            cdt_is_quad_edge_constrained(cdt_quad_edge *edge);

static int            cdt_get_vertex_count(Cdt_Context *ctx);
static int            cdt_get_edge_count(Cdt_Context *ctx);
static int            cdt_get_triangle_count(Cdt_Context *ctx);

static void           cdt_get_all_triangles(Cdt_Context *ctx, cdt_triangle *out_triangles);
static cdt_triangle   cdt_get_triangle_containing_point(Cdt_Context *ctx, f32 x, f32 y);
static cdt_triangles  cdt_get_adjacent_triangles(cdt_triangle triangle);
static cdt_quad_edge *cdt_get_portal_edge(cdt_triangle src, cdt_triangle dst);
