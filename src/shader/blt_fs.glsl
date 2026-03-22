// Copyright Seong Woo Lee. All Rights Reserved.

R"(

layout(binding=0) uniform sampler2D bitmap;
in vec2 fUV;
out vec4 result;

void main()
{
    result = texture(bitmap, fUV);
}

)";
