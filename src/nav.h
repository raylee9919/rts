/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */



    

internal v3
get_centroid(Navmesh *navmesh, int triangleindex) {
    v3 result = {};
    for (int i = 0; i < 3; ++i) {
        result += navmesh->vertices[navmesh->cdt.tri[triangleindex][i]].position;
    }
    result *= 0.333333f;
    return result;
}

internal f32
centroid_distance(Navmesh *navmesh, int a, int b) {
    v3 ca = get_centroid(navmesh, a);
    v3 cb = get_centroid(navmesh, b);
    f32 result = distance(ca, cb);
    return result;
}

internal b32
ssf_on_left(v3 p, v3 a, v3 b) 
{
    f32 det = (b.z-a.z)*(p.x-a.x) - (b.x-a.x)*(p.z-a.z);
    if (det >= 0) return true;
    return false;
}

internal b32
ssf_on_right(v3 p, v3 a, v3 b) 
{
    f32 det = (b.z-a.z)*(p.x-a.x) - (b.x-a.x)*(p.z-a.z);
    if (det <= 0) return true;
    return false;
}

internal b32
ssf_equal(v3 a, v3 b) 
{
    if (a.x == b.x && a.y == b.y && a.z == b.z) return true;
    return false;
}

// @NOTE: Reference: Efficient Triangulation-Based Pathfinding -Douglas Jon Demyen
internal b32
is_obtuse(v2 a, v2 b, v2 c)
{
    v2 p = a-b;
    v2 q = c-b;
    if (dot(p, q) < 0) return true;
    return false;
}

internal f32
search_width(Navmesh *navmesh, v2 C, int T, int E, f32 d)
{
    Cdt_Result *cdt = &navmesh->cdt;

    int Ui = cdt->tri[T][E];
    int Vi = cdt->tri[T][(E+1)%3];

    v2 U = v2{navmesh->vertices[Ui].position.z, navmesh->vertices[Ui].position.x};
    v2 V = v2{navmesh->vertices[Vi].position.z, navmesh->vertices[Vi].position.x};

    if (is_obtuse(C, U, V) || is_obtuse(C, V, U)) {
        return d;
    }

    f32 d2 = point_line_distance(C, U, V);

    if (d2 > d) {
        return d;
    }

    int L = cdt->adj[T][E];
    if (L == -1) return d2;
    if (!cdt->trespassable[L]) return d2;

    int ELT = cdt_edge(cdt->adj, L, T);
    int E2 = (ELT+1)%3;
    int E3 = (ELT+2)%3;
    f32 d3 = search_width(navmesh, C, L, E2, d);
    f32 d4 = search_width(navmesh, C, L, E3, d3);

    return d4;
}

internal f32
calculate_width(Navmesh *navmesh, int T, int Ei, int Eo)
{
    Cdt_Result *cdt = &navmesh->cdt;

    int E;
    if (((Eo+1)%3) == Ei) {
        E = Ei;
    } else {
        E = Eo;
    }

    int Ci = cdt->tri[T][E];
    int Ai = cdt->tri[T][(E+1)%3];
    int Bi = cdt->tri[T][(E+2)%3];

    v2 C = v2{navmesh->vertices[Ci].position.z, navmesh->vertices[Ci].position.x};
    v2 A = v2{navmesh->vertices[Ai].position.z, navmesh->vertices[Ai].position.x};
    v2 B = v2{navmesh->vertices[Bi].position.z, navmesh->vertices[Bi].position.x};

    v2 a = C - B;
    v2 b = A - C;
    v2 c = B - A;

    f32 d = min(length(a), length(b));

    // Right or obtuse angle.
    if (dot(c, -b) <= 0 || dot(-c, a) <= 0) {
        return d;
    }

    // If edge AB is constrained.
    int ETL = (E+1)%3;
    int L = cdt->adj[T][ETL];
    if (L == -1) {
        return point_line_distance(C, A, B);
    }
    if (!cdt->trespassable[L]) {
        // @TODO: Test this function.
        return point_line_distance(C, A, B);
    }

    // If edge AB isn't constrained.
    return search_width(navmesh, C, T, ETL, d);
}
