// Copyright Seong Woo Lee. All Rights Reserved.

#include "./immediate.h"
#include "basic/log.h"

// @Todo: Is it too wasteful?
per_thread R_Quad immediate_quads[R_MAX_QUADS];
per_thread u32    num_immediate_quads;

void 
draw_quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3,
          vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3,
          vec4 c0, vec4 c1, vec4 c2, vec4 c3,
          Guid texture) 
{
  if (num_immediate_quads < R_MAX_QUADS) 
  {
    R_Quad quad = {};
    quad.vertices[0] = p0;
    quad.vertices[1] = p1;
    quad.vertices[2] = p2;
    quad.vertices[3] = p3;
    quad.uvs[0] = uv0;
    quad.uvs[1] = uv1;
    quad.uvs[2] = uv2;
    quad.uvs[3] = uv3;
    quad.colors[0] = c0;
    quad.colors[1] = c1;
    quad.colors[2] = c2;
    quad.colors[3] = c3;
    quad.texture = texture;
    immediate_quads[num_immediate_quads++] = quad;
  } 
  else 
  {
    log_warning(S("Number of immediate quads exceed '%u'"), R_MAX_QUADS);
  }
}

void 
draw_quad(f32 x, f32 y, f32 w, f32 h, 
          vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3,
          vec4 c0, vec4 c1, vec4 c2, vec4 c3, 
          Guid texture) 
{
  vec2 p0 = vec2(x, y);
  vec2 p1 = vec2(x, y + h);
  vec2 p2 = vec2(x + w, y);
  vec2 p3 = vec2(x + w, y + h);
  draw_quad(p0, p1, p2, p3, 
            uv0, uv1, uv2, uv3,
            c0, c1, c2, c3, 
            texture);
}

void 
draw_line(vec2 p0, vec2 p1, vec4 c, f32 thickness) 
{
  f32 theta = m_atan2(p1.y - p0.y, p1.x - p0.x);
  f32 ct    = m_cos(theta);
  f32 st    = m_sin(theta);
  vec2 perp = vec2(-st, ct) * thickness;
  draw_quad(p0 - perp, p0 + perp, p1 - perp, p1 + perp, 
            {0,0}, {0,1}, {1,0}, {1,1}, // don't care
            c, c, c, c, 
            NULL_GUID);
}

void 
draw_box(AABB box, m4x4 view_proj, vec4 color, f32 thickness)
{
  vec3 lo = box.min;
  vec3 hi = box.max;

  // Corner i: bit 0 -> x, bit 1 -> y, bit 2 -> z (0 = min, 1 = max)
  vec2 px[8];
  b32  visible[8];
  for (int i = 0; i < 8; ++i)
  {
    vec3 c = vec3((i & 1) ? hi.x : lo.x,
                  (i & 2) ? hi.y : lo.y,
                  (i & 4) ? hi.z : lo.z);
    vec4 clip = view_proj * vec4(c, 1.f);
    visible[i] = clip.w > 0.f;
    vec2 ndc = clip.xy * (1.f/clip.w);
    px[i] = vec2((ndc.x*0.5f + 0.5f) * 2560, (0.5f - ndc.y*0.5f) * 1440);
  }

  // 12 edges: connect corners whose indices differ in exactly one bit
  for (int i = 0; i < 8; ++i)
  {
    for (int bit = 1; bit < 8; bit <<= 1)
    {
      if (i & bit) continue;
      int j = i | bit;
      if (visible[i] && visible[j])
        draw_line(px[i], px[j], color, thickness);
    }
  }
}
