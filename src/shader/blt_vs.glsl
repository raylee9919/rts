// Copyright Seong Woo Lee. All Rights Reserved.

R"(

layout (location = 0) in vec3 vP;
layout (location = 2) in vec2 vUV;

out vec2 fUV;

void main()
{
    fUV = vUV;
    gl_Position = v4(vP, 1.0f);
}

)";
