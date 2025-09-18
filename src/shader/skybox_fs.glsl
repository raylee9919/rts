/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

in v3 fUV;

out v4 result;

uniform samplerCube skybox;

void main()
{
    result = texture(skybox, fUV);
    //result = v4(1,1,1,1);
}

)";
