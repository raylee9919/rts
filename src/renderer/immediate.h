// Copyright Seong Woo Lee. All Rights Reserved.

#ifndef RTS_RENDERER_IMMEDIATE_H
#define RTS_RENDERER_IMMEDIATE_H

#include "math/math.h"
#include "variables.h"

struct R_Quad {
    vec2 vertices[4];
    vec2 uvs[4];
    vec4 colors[4];
};

void draw_quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3,
               vec4 c0, vec4 c1, vec4 c2, vec4 c3);

void draw_quad(f32 x, f32 y, f32 w, f32 h, 
               vec4 c0, vec4 c1, vec4 c2, vec4 c3);

void draw_line(vec2 p0, vec2 p1, vec4 c, f32 thickness);

extern per_thread R_Quad immediate_quads[R_MAX_QUADS];
extern per_thread u32    num_immediate_quads;

#endif // RTS_RENDERER_IMMEDIATE_H
