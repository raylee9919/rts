R"(

uniform m4x4 model;

layout (location = 0) in v3 vP;
layout (location = 1) in v3 vN;
layout (location = 2) in v2 vUV;
layout (location = 3) in v4 vC;
layout (location = 4) in v3 v_tangent;

out v3 cP;
out v3 cN;
out v2 cUV;
out v4 cC;
out m3x3 cTBN;

void main()
{
    cN           = normalize(m3x3(model) * vN);
    v3 tangent   = normalize(m3x3(model) * v_tangent);
    v3 bitangent = normalize(cross(cN, tangent));
    cTBN = m3x3(tangent, bitangent, cN);

    v4 world_position = model * v4(vP, 1.0f);
    cP = world_position.xyz;

    cUV = vUV;
    cC  = vC;

    gl_Position = world_position;
}

)";
