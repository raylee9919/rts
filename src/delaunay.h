/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Sung Woo Lee $
   $Notice: (C) Copyright 2025 by Sung Woo Lee. All Rights Reserved. $
   ======================================================================== */



struct Nav_Constrain {
    int (*edges)[2];
    u32 edge_count;
    u32 edge_size;
};

struct Cdt_Result {
    int numtri;
    int (*tri)[3];
    int (*adj)[3];
    s32 *trespassable;
};

struct Nav_Vertex {
    int idx; // if it is < 0, it isn't in the array.
    v3 position;
};
struct Nav_Portal {
    Nav_Vertex left;
    Nav_Vertex right;
};

struct Navmesh {
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


static u32
cdt_partition_bin(int *VIDX, u32 lo, u32 hi, int *BIN) 
{
    u32 j, k = lo;

    for (j = lo + 1; j < hi; j++) {
        if (BIN[VIDX[j]] - BIN[VIDX[lo]] <= 0) {
            k++;
            int tmp = VIDX[k];
            VIDX[k] = VIDX[j];
            VIDX[j] = tmp;
        }
    }

    int tmp = VIDX[k];
    VIDX[k] = VIDX[lo];
    VIDX[lo] = tmp;
    return k;
}

static void
cdt_quicksort_bin(int *VIDX, u32 lo, u32 hi, int *BIN) 
{
    if (hi > lo + 1) {
        u32 mid = cdt_partition_bin(VIDX, lo, hi, BIN);
        cdt_quicksort_bin(VIDX, lo, mid, BIN);
        cdt_quicksort_bin(VIDX, mid + 1, hi, BIN);
    }
}

static bool
cdt_point_in_triangle(v2 p, v2 a, v2 b, v2 c) 
{
    v2 ab = b - a;
    v2 bc = c - b;
    v2 ca = a - c;
    v2 ap = p - a;
    v2 bp = p - b;
    v2 cp = p - c;

    v2 n1 = v2{ab.y, -ab.x};
    v2 n2 = v2{bc.y, -bc.x};
    v2 n3 = v2{ca.y, -ca.x};

    f32 s1 = ap.x*n1.x + ap.y*n1.y;
    f32 s2 = bp.x*n2.x + bp.y*n2.y;
    f32 s3 = cp.x*n3.x + cp.y*n3.y;

    f32 tolerance = 0.0001f;

    if ((s1 < 0 && s2 < 0 && s3 < 0) ||
        (s1 < tolerance && s2 < 0 && s3 < 0) ||
        (s2 < tolerance && s3 < 0 && s1 < 0) || 
        (s3 < tolerance && s1 < 0 && s2 < 0)) {
        return true;
    } else {
        return false;
    }
}

// @TODO: Identical vertices cases not handled. Meaning, there can be identical triangles with
// different id, meaning, there might be more than 3 adjacent triangeles.
static int 
cdt_edge(int (*adj)[3], int L, int K) 
{
    for (int e = 0; e < 3; ++e) {
        if (adj[L][e] == K) {
            return e;
            break;
        }
    }
    INVALID_CODE_PATH;
    return -1;
}

// @NOTE: Colinear isn't considered on left.
internal b32
point_on_left(v2 p, v2 a, v2 b, b32 colinear = false) 
{
    v2 v1 = b-a;
    v2 v2 = p-a;
    f32 det = v1.x*v2.y - v1.y*v2.x;
    if (det > 0) return true;
    if (colinear && det == 0) return true;
    return false;
}

// @NOTE: Colinear isn't considered on left.
internal b32
point_on_right(v2 p, v2 a, v2 b, b32 colinear = false) 
{
    v2 v1 = b-a;
    v2 v2 = p-a;
    f32 det = v1.x*v2.y - v1.y*v2.x;
    if (det < 0) return true;
    if (colinear && det == 0) return true;
    return false;
}

// @NOTE: Colinear points aren't considered intersecting.
static bool
cdt_intersect(v2 a1, v2 a2, v2 b1, v2 b2) 
{
    v2 p = a2-a1;
    v2 q1 = b1-a2;
    v2 q2 = b2-a2;
    float c1 = p.x*q1.y - p.y*q1.x;
    float c2 = p.x*q2.y - p.y*q2.x;
    if (c1*c2 >= 0) return false;

    v2 q = b2-b1;
    v2 p1 = a1-b2;
    v2 p2 = a2-b2;
    c1 = q.x*p1.y - q.y*p1.x;
    c2 = q.x*p2.y - q.y*p2.x;
    if (c1*c2 >= 0) return false;

    return true;
}

static bool
is_convex(v2 a, v2 b, v2 c, v2 d) 
{
    v2 s[4];
    s[0] = b-a;
    s[1] = c-b;
    s[2] = d-c;
    s[3] = a-d;

    float cp[4];
    for (int i = 0; i < 4; ++i) {
        int j = (i+1)%4;
        cp[i] = (s[i].x*s[j].y - s[i].y*s[j].x);
    }

    if (cp[0] < 0 && cp[1] < 0 && cp[2] < 0 && cp[3] < 0) return true;
    if (cp[0] > 0 && cp[1] > 0 && cp[2] > 0 && cp[3] > 0) return true;
    return false;
}

bool cdt_bad(float xp, float x1, float x2, float x3, float yp, float y1, float y2, float y3) 
{
    // @NOTE: Determine if a pair of adjacent triangles form a convex quadrilateral with the maximum minimum angle.

    float x13 = x1 - x3;
    float x23 = x2 - x3;
    float x1p = x1 - xp;
    float x2p = x2 - xp;

    float y13 = y1 - y3;
    float y23 = y2 - y3;
    float y1p = y1 - yp;
    float y2p = y2 - yp;

    float cosa = x13*x23 + y13*y23;
    float cosb = x2p*x1p + y2p*y1p;

    bool result;

    if (cosa >= 0 && cosb >= 0) {
        result = false;
    } else if (cosa < 0 && cosb < 0) {
        result = true;
    } else {
        float sina = x13*y23 - x23*y13;
        float sinb = x2p*y1p - x1p*y2p;

        float sinab = sina*cosb + sinb*cosa;

        if (sinab < 0) {
            result = true;
        } else {
            result = false;
        }
    }

    return result;
}

// @TODO: Remove malloc()
internal Cdt_Result
delaunay_triangulate(Vertex *vertices, u32 vertexcount, Navmesh *navmesh)
{
#if __DEVELOPER
    u64 tsc_begin = os.read_cpu_timer();
#endif

    int count = (int)vertexcount;

    int *VIDX = (int *)malloc(sizeof(int)*count);
    scope_exit(free(VIDX));
    for (int i = 0; i < count; ++i) VIDX[i] = i;

    int *BIN = (int *)malloc(sizeof(int)*count);
    scope_exit(free(BIN));

    v2 *positions = (v2 *)malloc(sizeof(v2)*(count+3));
    scope_exit(free(positions));
    for (int i = 0; i < count; ++i) {
        positions[i].x = vertices[i].position.z;
        positions[i].y = vertices[i].position.x;
    }

    // @NOTE: Normalize while keeping aspect ratio.
    float xmin =  F32_MAX;
    float xmax = -F32_MAX;
    float ymin =  F32_MAX;
    float ymax = -F32_MAX;

    for (int i = 0; i < count; ++i) {
        float x = positions[i].x;
        float z = positions[i].y;
        xmin = min(xmin, x);
        xmax = max(xmax, x);
        ymin = min(ymin, z);
        ymax = max(ymax, z);
    }

    float invdmax = 1.0f / max(xmax - xmin, ymax - ymin);

    for (int i = 0; i < count; ++i) {
        positions[i].x = (positions[i].x - xmin) * invdmax;
        positions[i].y = (positions[i].y - ymin) * invdmax;
    }

    float invnxmax = 1.0f / ((xmax - xmin) * invdmax);
    float invnymax = 1.0f / ((ymax - ymin) * invdmax);

    // @NOTE: Sort by proximity.
    int ndiv = (int)(pow((float)count, 0.25f) + 0.5f);
    for (int k = 0; k < count; ++k) {
        float x = positions[k].x;
        float y = positions[k].y;
        int i = int(y*ndiv*0.99f*invnxmax);
        int j = int(x*ndiv*0.99f*invnymax);
        int bin = (i % 2 == 0) ? i*ndiv+j+1 : (i+1)*ndiv-j;
        BIN[k] = bin;
    }
    cdt_quicksort_bin(VIDX, 0, count, BIN);

    // Add super-triangle to vertex array.
    positions[count].x   = -100;
    positions[count].y   = -100;

    positions[count+1].x = 100;
    positions[count+1].y = -100;

    positions[count+2].x = 0;
    positions[count+2].y = 100;

    count += 3;

    int maxnumtri = 2*(count+3);

    int (*tri)[3] = (int (*)[3])malloc(3*sizeof(int)*maxnumtri);
    scope_exit(free(tri));
    tri[0][0] = count-3;
    tri[0][1] = count-2;
    tri[0][2] = count-1;

    int (*adj)[3] = (int (*)[3])malloc(3*sizeof(int)*maxnumtri);
    scope_exit(free(adj));
    adj[0][0] = -1;
    adj[0][1] = -1;
    adj[0][2] = -1;

    s32 *trespassable = (s32 *)malloc(sizeof(s32)*maxnumtri);
    scope_exit(free(trespassable));
    for (int i = 0; i < maxnumtri; ++i) trespassable[i] = 1;

    int maxstk = (count-3);
    int *ts = (int *)malloc(maxstk*sizeof(int)); // @NOTE: Sloan suggests #point is good enough for 10,000. Assertion required.
    scope_exit(free(ts));
    int top = -1;

    int numtri = 1;

    for (int vii = 0; vii < count - 3; ++vii) {
        int p = VIDX[vii];
        //int ti = numtri - 1;
        for (int ti = numtri - 1; ti >= 0; --ti) {
            if (cdt_point_in_triangle(positions[p], positions[tri[ti][0]], positions[tri[ti][1]], positions[tri[ti][2]])) {
                tri[numtri][0] = p;
                tri[numtri][1] = tri[ti][1];
                tri[numtri][2] = tri[ti][2];

                tri[numtri+1][0] = p;
                tri[numtri+1][1] = tri[ti][2];
                tri[numtri+1][2] = tri[ti][0];

                // Retrive adj info.
                int A = adj[ti][0];
                int B = adj[ti][1];
                int C = adj[ti][2];

                // Update adj info of surrounding triangles.
                if (B >= 0) {
                    int EBT = cdt_edge(adj, B, ti);
                    adj[B][EBT] = numtri;
                }

                if (C >= 0) {
                    int ECT = cdt_edge(adj, C, ti);
                    adj[C][ECT] = numtri+1;
                }

                // Update adj of new triangles.
                adj[ti][0] = numtri+1;
                adj[ti][1] = A;
                adj[ti][2] = numtri;

                adj[numtri][0] = ti;
                adj[numtri][1] = B;
                adj[numtri][2] = numtri+1;

                adj[numtri+1][0] = numtri;
                adj[numtri+1][1] = C;
                adj[numtri+1][2] = ti;

                tri[ti][2] = tri[ti][1];
                tri[ti][1] = tri[ti][0];
                tri[ti][0] = p;

                // Push newly added triangles to stack if has opposing triangle.
                if (adj[ti][1] >= 0) {
                    Assert(top < maxstk-1);
                    ts[++top] = ti;
                }

                if (adj[numtri][1] >= 0) {
                    Assert(top < maxstk-1);
                    ts[++top] = numtri;
                }

                if (adj[numtri+1][1] >= 0) {
                    Assert(top < maxstk-1);
                    ts[++top] = numtri+1;
                }

                while (top >= 0) {
                    int L = ts[top--];
                    int R = adj[L][1];
                    Assert(R >= 0);

                    int ERL = cdt_edge(adj, R, L);
                    int ERA = (ERL + 1) % 3;
                    int ERB = (ERL + 2) % 3;

                    int P  = tri[L][0];
                    int V1 = tri[R][ERL];
                    int V2 = tri[R][ERA];
                    int V3 = tri[R][ERB];

                    float xp = positions[P].x;
                    float x1 = positions[V1].x;
                    float x2 = positions[V2].x;
                    float x3 = positions[V3].x;

                    float yp = positions[P].y;
                    float y1 = positions[V1].y;
                    float y2 = positions[V2].y;
                    float y3 = positions[V3].y;

                    if (cdt_bad(xp,x1,x2,x3,yp,y1,y2,y3)) {
                        int A = adj[R][ERA];
                        int B = adj[R][ERB];
                        int C = adj[L][2];

                        // Update vertex and adjacency list for L.
                        tri[L][2] = V3;
                        adj[L][1] = A;
                        adj[L][2] = R;

                        // Update vertex and adjacency list for R.
                        tri[R][0] = P;
                        tri[R][1] = V3;
                        tri[R][2] = V1;
                        adj[R][0] = L;
                        adj[R][1] = B;
                        adj[R][2] = C;

                        // Push L-A and R-B on stack.
                        // Update adjacency lists for triangle A and C.
                        if (A >= 0) {
                            int EAR = cdt_edge(adj, A, R);
                            adj[A][EAR] = L;
                            Assert(top < maxstk-1);
                            ts[++top] = L;
                        }
                        if (B >= 0) {
                            Assert(top < maxstk-1);
                            ts[++top] = R;
                        }
                        if (C >= 0) {
                            int ECL = cdt_edge(adj, C, L);
                            adj[C][ECL] = R;
                        }
                    }
                }

                numtri += 2;

                break;
            }
        }

        // @TODO: adjust triangle index according to direction and sorted bin.
    }

    // @NOTE: Check consistency of triangulation.
    Assert(numtri == 2*(count-3)+1);

    // @NOTE: Constrained Delaunay Triangulation.
    Stack<Pair<int, int>> removestack = {};

    if (navmesh->constrain_count > 0) {
        int *tl = (int *)malloc(sizeof(int)*count*numtri);
        scope_exit(free(tl));

        int *tlcount = (int *)malloc(sizeof(int)*count);
        scope_exit(free(tlcount));
        zero_array(tlcount, count);

        // Itersecting edges.
        int ielen = count*3;
        int (*ie)[2] = (int (*)[2])malloc(2*sizeof(int)*ielen);
        int ielo = 0;
        int iehi = 0;
        scope_exit(free(ie));

        // New edges.
        int nelen = count*3;
        int (*ne)[2] = (int (*)[2])malloc(2*sizeof(int)*nelen);
        int nelo = 0;
        int nehi = 0;
        scope_exit(free(ne));

        // Build triangle-list per vertex.
        int tlpitch = numtri;
        for (int T = 0; T < numtri; ++T) {
            for (int i = 0; i < 3; ++i) {
                int V = tri[T][i];
                *(tl + (V*tlpitch) + tlcount[V]++) = T;
            }
        }


        // @NOTE: Iterate through constraint edges.
        for (u32 constrain_idx = 0; constrain_idx < navmesh->constrain_count; ++constrain_idx) {
            Nav_Constrain *constrain = navmesh->constrains + constrain_idx;
            for (u32 cei = 0; cei < constrain->edge_count; ++cei) {
                int vi = constrain->edges[cei][0];
                int vj = constrain->edges[cei][1];


                // @NOTE: Skip if the edge is already in the triangulation.
                int T = -1;
                bool terminate = false;
                bool alreadyin = false;
                for (int ti = 0; ti < tlcount[vi]; ++ti) {
                    int tmpT = tl[vi*tlpitch + ti];

                    int vii = -1;
                    for (int i = 0; i < 3; ++i) {
                        if (tri[tmpT][i] == vi) {
                            vii = i;
                            break;
                        }
                    }
                    Assert(vii != -1);

                    for (int v = 0; v < 3; ++v) {
                        if (tri[tmpT][v] == vj) {
                            alreadyin = true;
                            terminate = true;
                            break;
                        } else if (point_on_left(positions[vj], positions[tri[tmpT][vii]], positions[tri[tmpT][(vii+1)%3]]) &&
                                   point_on_left(positions[vj], positions[tri[tmpT][(vii+2)%3]], positions[tri[tmpT][vii]])) {
                            terminate = true;
                            T = tmpT;
                            break;
                        }
                    }

                    if (terminate) {
                        break;
                    }
                }

                if (alreadyin) {
                    continue;
                }

                Assert(T != -1);

                while (1) {
                    if (tri[T][0] == vj || tri[T][1] == vj || tri[T][2] == vj) {
                        break;
                    }

                    for (int v = 0; v < 3; ++v) {
                        int vk = tri[T][v];
                        int vl = tri[T][(v+1)%3];
                        if (!point_on_left(positions[vj], positions[vk], positions[vl])) {
                            if (cdt_intersect(positions[vi], positions[vj], positions[vk], positions[vl])) {
                                ie[iehi][0] = vk;
                                ie[iehi][1] = vl;
                                iehi = (iehi+1)%ielen;

                                T = adj[T][v];
                                break;
                            }
                        }
                    }
                }

                // @NOTE Iterate intersecting edges (ie).
                while (ielo != iehi) {
                    // @NOTE: Remove an edge from the list.
                    int vk = ie[ielo][0];
                    int vl = ie[ielo][1];

                    ielo = (ielo+1)%ielen;

                    int L = -1;
                    int R = -1;

                    for (int i = 0; i < tlcount[vk]; ++i) {
                        int T = tl[vk*tlpitch + i];
                        for (int v1 = 0; v1 < 3; ++v1) {
                            int v2 = (v1+1)%3;
                            if ((tri[T][v1] == vk && tri[T][v2] == vl)) {
                                L = T;
                                break;
                            }

                            if ((tri[T][v1] == vl && tri[T][v2] == vk)) {
                                R = T;
                                break;
                            }
                        }

                        if (L != -1 && R != -1) break;
                    }

                    int ELR = cdt_edge(adj, L, R);
                    int ERL = cdt_edge(adj, R, L);
                    int ERA = (ERL+1)%3;
                    int ERB = (ERL+2)%3;
                    int ELC = (ELR+1)%3;
                    int ELD = (ELR+2)%3;

                    int V2 = tri[L][ELR];
                    int V1 = tri[L][(ELR+1)%3];
                    int P = tri[L][(ELR+2)%3];
                    int V3 = tri[R][(ERL+2)%3];

                    if (!is_convex(positions[P], positions[V2], positions[V3], positions[V1])) {
                        if (cdt_intersect(positions[vi], positions[vj], positions[V1], positions[V2])) {
                            // @NOTE: If strictly concave, and still intersects, put it back on.
                            ie[iehi][0] = vk;
                            ie[iehi][1] = vl;
                            iehi = (iehi+1)%ielen;
                        }
                    } else {
                        // @NOTE: If convex, swap diagonal.
                        int A = adj[R][ERA];
                        int B = adj[R][ERB];
                        int C = adj[L][ELC];
                        int D = adj[L][ELD];

                        // Update vertex and adjacency list for L.
                        tri[L][0] = P;
                        tri[L][1] = V2;
                        tri[L][2] = V3;
                        adj[L][0] = D;
                        adj[L][1] = A;
                        adj[L][2] = R;

                        // Update vertex and adjacency list for R.
                        tri[R][0] = P;
                        tri[R][1] = V3;
                        tri[R][2] = V1;
                        adj[R][0] = L;
                        adj[R][1] = B;
                        adj[R][2] = C;

                        // Update adjacency lists for triangle A and C.
                        if (A >= 0) {
                            int EAR = cdt_edge(adj, A, R);
                            adj[A][EAR] = L;
                        }
                        if (C >= 0) {
                            int ECL = cdt_edge(adj, C, L);
                            adj[C][ECL] = R;
                        }

                        // @NOTE: Update triangle-list.
                        //        Remove L from V1, Add L to V3
                        //        Remove R from V2, Add R to P 
                        for (int i = 0; i < tlcount[V1]; ++i) {
                            if (tl[V1*tlpitch + i] == L) {
                                tl[V1*tlpitch + i] = tl[V1*tlpitch + --tlcount[V1]];
                                break;
                            }
                        }
                        tl[V3*tlpitch + tlcount[V3]++] = L;

                        for (int i = 0; i < tlcount[V2]; ++i) {
                            if (tl[V2*tlpitch + i] == R) {
                                tl[V2*tlpitch + i] = tl[V2*tlpitch + --tlcount[V2]];
                                break;
                            }
                        }
                        tl[P*tlpitch + tlcount[P]++] = R;

                        // @NOTE: If still intersects, add to intersecting list.
                        int ELR = cdt_edge(adj, L, R);
                        if (cdt_intersect(positions[vi], positions[vj], positions[P], positions[V3])) {
                            ie[iehi][0] = V3;
                            ie[iehi][1] = P;
                            iehi = (iehi+1)%ielen;
                        } else {
                            // @NOTE: If not, place it on a list of newly created edges.
                            ne[nehi][0] = V3;
                            ne[nehi][1] = P;
                            nehi = (nehi+1)%nelen;
                        }
                    }
                }

                // @NOTE: Iterate over newly created edges.
                while (nelo != nehi) {
                    int vk = ne[nelo][0];
                    int vl = ne[nelo][1];

                    nelo = (nelo+1)%nelen;

                    int L = -1;
                    int R = -1;

                    for (int i = 0; i < tlcount[vk]; ++i) {
                        int T = tl[vk*tlpitch + i];
                        for (int v1 = 0; v1 < 3; ++v1) {
                            int v2 = (v1+1)%3;
                            if ((tri[T][v1] == vk && tri[T][v2] == vl)) {
                                L = T;
                                break;
                            }

                            if ((tri[T][v1] == vl && tri[T][v2] == vk)) {
                                R = T;
                                break;
                            }
                        }

                        if (L != -1 && R != -1) break;
                    }

                    int ELR = cdt_edge(adj, L, R);
                    int ERL = cdt_edge(adj, R, L);
                    int ERA = (ERL+1)%3;
                    int ERB = (ERL+2)%3;
                    int ELC = (ELR+1)%3;
                    int ELD = (ELR+2)%3;

                    int V2 = tri[L][ELR];
                    int V1 = tri[L][(ELR+1)%3];
                    int P = tri[L][(ELR+2)%3];
                    int V3 = tri[R][(ERL+2)%3];

                    float xp = positions[P].x;
                    float x1 = positions[V1].x;
                    float x2 = positions[V2].x;
                    float x3 = positions[V3].x;

                    float yp = positions[P].y;
                    float y1 = positions[V1].y;
                    float y2 = positions[V2].y;
                    float y3 = positions[V3].y;

                    // @NOTE: If it's constrained edge, skip.
                    if ((vk==vi && vl==vj) || (vk==vj && vl==vi)) {
                        continue;
                    }

                    // @NOTE: If delaunay-bad, swap.
                    if (cdt_bad(xp,x1,x2,x3,yp,y1,y2,y3)) {
                        int A = adj[R][ERA];
                        int B = adj[R][ERB];
                        int C = adj[L][ELC];
                        int D = adj[L][ELD];

                        // Update vertex and adjacency list for L.
                        tri[L][ELD] = P;
                        tri[L][(ELD+1)%3] = V2;
                        tri[L][(ELD+2)%3] = V3;
                        adj[L][ELD] = D;
                        adj[L][(ELD+1)%3] = A;
                        adj[L][(ELD+2)%3] = R;

                        // Update vertex and adjacency list for R.
                        tri[R][ERB] = V3;
                        tri[R][(ERB+1)%3] = V1;
                        tri[R][(ERB+2)%3] = P;
                        adj[R][ERB] = B;
                        adj[R][(ERB+1)%3] = C;
                        adj[R][(ERB+2)%3] = L;

                        // Update adjacency lists for triangle A and C.
                        if (A >= 0) {
                            int EAR = cdt_edge(adj, A, R);
                            adj[A][EAR] = L;
                        }
                        if (C >= 0) {
                            int ECL = cdt_edge(adj, C, L);
                            adj[C][ECL] = R;
                        }

                        // @NOTE: Update triangle-list.
                        //        Remove L from V1, Add L to V3
                        //        Remove R from V2, Add R to P 
                        for (int i = 0; i < tlcount[V1]; ++i) {
                            if (tl[V1*tlpitch + i] == L) {
                                tl[V1*tlpitch + i] = tl[V1*tlpitch + --tlcount[V1]];
                                break;
                            }
                        }
                        tl[V3*tlpitch + tlcount[V3]++] = L;

                        for (int i = 0; i < tlcount[V2]; ++i) {
                            if (tl[V2*tlpitch + i] == R) {
                                tl[V2*tlpitch + i] = tl[V2*tlpitch + --tlcount[V2]];
                                break;
                            }
                        }
                        tl[P*tlpitch + tlcount[P]++] = R;
                    }
                }
            }
        }

        // @NOTE: Make a hole.
        bool *visited = (bool *)malloc(sizeof(bool) * numtri);
        scope_exit(free(visited));

        // For each constrain polygon,
        for (u32 constrain_idx = 0; constrain_idx < navmesh->constrain_count; ++constrain_idx) {
            Nav_Constrain *constrain = navmesh->constrains + constrain_idx;
            zero_array(visited, numtri);
            Queue<int> queue = {};

            // For each constrain edges,
            for (u32 e = 0; e < constrain->edge_count; ++e) {
                // @NOTE: Find for starting BFS triangle.
                int vk = constrain->edges[e][0];
                int vl = constrain->edges[e][1];

                int startv = vk;
                if (tlcount[vl] < tlcount[vk]) startv = vl;

                int T = -1;
                for (int ti = 0; ti < tlcount[startv]; ++ti) {
                    int t = tl[startv*tlpitch + ti];
                    for (int i = 0; i < 3; ++i) {
                        if (tri[t][i] == vk && tri[t][(i+1)%3] == vl) {
                            T = t;
                            break;
                        }
                    }

                    if (T != -1) break;
                }
                Assert(T != -1);

                if (!visited[T]) {
                    enqueue(&queue, T);
                }
            }

            while (!empty(&queue)) {
                int T = dequeue(&queue);

                if (!visited[T]) {
                    visited[T] = true;

                    trespassable[T] = 0;

                    for (int i = 0; i < 3; ++i) {
                        int R = adj[T][i];
                        if (R != -1) {
                            bool in = false;

                            int vk = tri[R][i];
                            int vl = tri[R][(i+1)%3];

                            // @TODO: This is lunacy.
                            for (u32 j = 0; j < constrain->edge_count; ++j) {
                                if (constrain->edges[j][0] == vk && constrain->edges[j][1] == vl) {
                                    in = true;
                                    break;
                                }
                            }

                            if (in) {
                                if (!visited[R]) {
                                    enqueue(&queue, R);
                                }
                            }
                        }
                    }
                }
            }
        }
        // End of Making Hole.

    }


    // @NOTE: Remove triangles including super-triangle's vertex.
    bool *rmflag = (bool *)malloc(sizeof(bool)*numtri);
    scope_exit(free(rmflag));
    zero_array(rmflag, numtri);

    int *trimap = (int *)malloc(sizeof(int)*numtri);
    scope_exit(free(trimap));

    int result_numtri = numtri;
    for (int ti = 0; ti < numtri; ++ti) {
        if (tri[ti][0] >= count-3 || tri[ti][1] >= count-3 || tri[ti][2] >= count-3) {
            rmflag[ti] = 1;
            --result_numtri;
        }
    }

    int (*result_tri)[3] = (int (*)[3])malloc(sizeof(int)*3*result_numtri);
    int (*result_adj)[3] = (int (*)[3])malloc(sizeof(int)*3*result_numtri);
    s32 *result_trespassable = (s32 *)malloc(sizeof(s32)*result_numtri);

    int idx = 0;
    for (int T = 0; T < numtri; ++T) {
        if (rmflag[T]) {
            trimap[T] = -1;
        } else {
            result_tri[idx][0] = tri[T][0];
            result_tri[idx][1] = tri[T][1];
            result_tri[idx][2] = tri[T][2];

            trimap[T] = idx;

            ++idx;
        }
    }
    Assert(idx == result_numtri);

    for (int T = 0; T < numtri; ++T) {
        int idx = trimap[T];
        if (idx >= 0) {
            for (int i = 0; i < 3; ++i) {
                result_adj[idx][i] = trimap[adj[T][i]];
            }
            result_trespassable[idx] = trespassable[T];
        }
    }

    // @NOTE: Remap to original value.
    //count-=3;

#if __DEVELOPER
    u64 tsc_end = os.read_cpu_timer();
    f32 elapsed_ms = 1000.0f * (tsc_end-tsc_begin) / os.tsc_frequency;
    printf("Generated Navmesh in %.2fms.\n", elapsed_ms);
#endif

    Cdt_Result result = {};
    result.numtri = result_numtri;
    result.tri = result_tri;
    result.adj = result_adj;
    result.trespassable = result_trespassable;
    return result;
}

internal void
begin_constrain(Navmesh *nv)
{
    Assert(nv->constrain_count < nv->constrain_size);
    nv->is_constrain = true;
    nv->first_vertex = nv->vertex_count;
}

internal void
end_constrain(Navmesh *nv)
{
    Nav_Constrain *constrain = nv->constrains + nv->constrain_count;
    constrain->edges[constrain->edge_count - 1][1] = nv->first_vertex;

    nv->is_constrain = false;
    ++nv->constrain_count;
}

internal void
push_vertex(Navmesh *nv, v3 v) 
{
    Assert(nv->vertex_count < nv->vertex_size);

    if (nv->is_constrain) {
        Nav_Constrain *c = nv->constrains + nv->constrain_count;
        Assert(c->edge_count < c->edge_size);
        c->edges[c->edge_count][0] = nv->vertex_count;
        c->edges[c->edge_count][1] = (nv->vertex_count + 1) % nv->vertex_size;
        ++c->edge_count;
    }

    nv->vertices[nv->vertex_count++].position = v;
}
