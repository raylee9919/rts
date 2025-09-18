/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

layout(binding=0) uniform sampler2D bitmap;
in v2 fUV;
out v4 result;

void main()
{
    v4 texture_color = texture(bitmap, fUV);
    texture_color.rgb = pow(texture_color.rgb, v3(1.0/2.2));

    result = texture_color;
}

)";
