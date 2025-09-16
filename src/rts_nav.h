#ifndef RTS_NAV_H
#define RTS_NAV_H
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright 2025 by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

internal v3 get_centroid(Navmesh *navmesh, int triangleindex);
internal f32 centroid_distance(Navmesh *navmesh, int a, int b);
internal b32 ssf_on_left(v3 p, v3 a, v3 b);
internal b32 ssf_on_right(v3 p, v3 a, v3 b);
internal b32 ssf_equal(v3 a, v3 b);
internal b32 is_obtuse(v2 a, v2 b, v2 c);
internal f32 search_width(Navmesh *navmesh, v2 C, int T, int E, f32 d);
internal f32 calculate_width(Navmesh *navmesh, int T, int Ei, int Eo);


#endif // RTS_NAV_H
