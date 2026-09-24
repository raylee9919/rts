// Copyright Seong Woo Lee. All Rights Reserved.

#include "./draw.h"

Array<R_Quad> immediate_quads;

void draw_quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3,
               vec4 c0, vec4 c1, vec4 c2, vec4 c3) {
    R_Quad quad = {};
    quad.vertices[0] = p0;
    quad.vertices[1] = p1;
    quad.vertices[2] = p2;
    quad.vertices[3] = p3;
    quad.colors[0] = c0;
    quad.colors[1] = c1;
    quad.colors[2] = c2;
    quad.colors[3] = c3;
    array_add(&immediate_quads, quad);
}

void draw_quad(f32 x, f32 y, f32 w, f32 h, 
               vec4 c0, vec4 c1, vec4 c2, vec4 c3) 
{
    vec2 p0 = vec2(x, y);
    vec2 p1 = vec2(x, y + h);
    vec2 p2 = vec2(x + w, y);
    vec2 p3 = vec2(x + w, y + h);
    draw_quad(p0, p1, p2, p3,  c0, c1, c2, c3);
}
