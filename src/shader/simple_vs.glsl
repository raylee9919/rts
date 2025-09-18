/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

layout (location = 0) in v3 vP;

out v3 fP;

uniform m4x4 VP;

void main()
{
    fP = vP;
    gl_Position = VP * v4(vP, 1);
}

)";
