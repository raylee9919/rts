// Copyright Seong Woo Lee. All Rights Reserved.

R"(

in vec3 fUV;

out vec4 result;

layout(binding=0) uniform samplerCube skybox;

void main()
{
    result = texture(skybox, fUV);
}

)";
