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
in vec4 fC;

out vec4 result;

void main()
{
    vec4 texture_color = texture(texture_sample, fUV);

    result = texture_color * fC;
}

)";
