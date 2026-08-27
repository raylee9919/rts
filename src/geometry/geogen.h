// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_GEOMETRY_GENERATOR_H
#define RTS_GEOMETRY_GENERATOR_H

#include "./cdt.h"

internal void 
geo_make_cube(void *out_vertices, int vertex_stride,
              int position_offset,
              int normal_offset,
              int uv_offset,
              void *out_indices, int index_size);

#endif // RTS_GEOMETRY_GENERATOR_H
