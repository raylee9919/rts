/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

layout (location = 0) in vec2   vP;
layout (location = 2) in vec2   vUV;
layout (location = 3) in vec4   vC;

smooth out vec2 fUV;
smooth out vec4 fC;

uniform float viewport_w;
uniform float viewport_h;

void main()
{
    fUV = vec2(vUV.x, vUV.y);
    fC  = vC;

    // # Hack:
    float x = ( vP.x / viewport_w * 2.f) - 1.0f;
    float y = (-vP.y / viewport_h * 2.f) + 1.0f;
    gl_Position = vec4(x, y, 0.0f, 1.0f);
}

)";
