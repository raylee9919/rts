/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(


uniform sampler2D u_sampler;

smooth in vec2  f_position;
smooth in vec2  f_uv;
smooth in vec4  f_color;
flat   in vec2  f_rect_center;
flat   in vec2  f_rect_half_dim;
flat   in float f_radius;

out vec4 out_color;


void main()
{
    vec2 d = abs(f_position - f_rect_center) - f_rect_half_dim - v2(f_radius);
    float s = length(max(d, 0.0f)) + min(max(d.x, d.y), 0.0f) - f_radius;

    vec4 texture_color = texture(u_sampler, f_uv);
    //out_color = texture_color * f_color;
    //out_color = vec4(f_radius/20.f,0,0,1);
}

)";
