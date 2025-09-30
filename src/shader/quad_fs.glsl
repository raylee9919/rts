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
in v4 fC;

out vec4 C;

void main()
{
    vec4 texture_color = texture(texture_sample, fUV);

    C = texture_color * fC;
}

)";
