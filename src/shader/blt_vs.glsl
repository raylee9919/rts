/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

layout (location = 0) in v3 vP;
layout (location = 2) in v2 vUV;

out v2 fUV;

void main()
{
    fUV = vUV;
    gl_Position = v4(vP, 1.0f);
}

)";
