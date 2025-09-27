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

smooth out vec2 fUV;

void main()
{
    fUV = vec2(vUV.x, vUV.y);

    // # Hack:
    float x = ( vP.x / 960.0f) - 1.0f;
    float y = (-vP.y / 540.0f) + 1.0f;
    gl_Position = vec4(x, y, 0.0f, 1.0f);
}

)";
