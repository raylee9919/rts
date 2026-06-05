// Copyright Seong Woo Lee. All Rights Reserved.

R"(

layout (location = 0) in vec2 position;

out vec2 uv;

void main()
{
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);

    uv = pos;

    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}

)";
