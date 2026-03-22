// Copyright Seong Woo Lee. All Rights Reserved.

R"(

layout (location = 0) in vec3 vP;

out vec3 fUV;

uniform mat4 view_proj;

void main()
{
    fUV = vP;
    gl_Position = view_proj * vec4(vP, 1);
}


)";
