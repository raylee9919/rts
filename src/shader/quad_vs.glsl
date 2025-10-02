/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

layout (location = 0) in vec2    v_position;
layout (location = 2) in vec2    v_uv;
layout (location = 3) in vec4    v_color;
layout (location = 4) in vec2    v_rect_center;
layout (location = 5) in vec2    v_rect_half_dim;
layout (location = 6) in float   v_radius;

smooth out vec2  f_position;
smooth out vec2  f_uv;
smooth out vec4  f_color;
flat   out vec2  f_rect_center;
flat   out vec2  f_rect_half_dim;
flat   out float f_radius;


uniform float viewport_w;
uniform float viewport_h;

void main()
{
    f_position = v_position;
    f_uv = vec2(v_uv.x, v_uv.y);
    f_color  = v_color;
    f_rect_center = v_rect_center;
    f_rect_half_dim = v_rect_half_dim;
    f_radius = v_radius;

    float x = ( v_position.x / viewport_w * 2.0f) - 1.0f;
    float y = (-v_position.y / viewport_h * 2.0f) + 1.0f;
    gl_Position = vec4(x, y, 0.0f, 1.0f);
}

)";
