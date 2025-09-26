/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

uniform sampler2D texture_sample;

in vec2 fUV;
out vec4 C;

void main()
{
    vec4 const_color = vec4(0,1,1,1);
    vec4 texture_color = texture(texture_sample, fUV);

    C = vec4(fUV, 0, 1);
}

)";
