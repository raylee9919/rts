R"(

layout (location = 0) in v3 vP;

out v3 fUV;

uniform m4x4 view_proj;

void main()
{
    fUV = vP;
    gl_Position = view_proj * v4(vP, 1);
}


)";
