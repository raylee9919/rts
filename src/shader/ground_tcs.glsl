R"(

layout (vertices = 4) out;

in v3 cP[];
in v3 cN[];
in v2 cUV[];
in v4 cC[];
in m3x3 cTBN[];

out v3 eP[];
out v3 eN[];
out v2 eUV[];
out v4 eC[];
out m3x3 eTBN[];

uniform v3 eye_position;

void main()
{
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    eP[gl_InvocationID]   = cP[gl_InvocationID];
    eN[gl_InvocationID]   = cN[gl_InvocationID];
    eUV[gl_InvocationID]  = cUV[gl_InvocationID];
    eC[gl_InvocationID]   = cC[gl_InvocationID];
    eTBN[gl_InvocationID] = cTBN[gl_InvocationID];

    f32 dist = distance(eye_position, cP[0]);

    if (gl_InvocationID == 0) {
        const u32 min_tess_level = 1;
        const u32 max_tess_level = 32;
        const f32 min_distance = 10;
        const f32 max_distance = 200;

        f32 dist00 = distance(cP[0], eye_position);
        f32 dist01 = distance(cP[1], eye_position);
        f32 dist10 = distance(cP[2], eye_position);
        f32 dist11 = distance(cP[3], eye_position);

        f32 t00 = clamp(((max_distance - dist00) / (max_distance - min_distance)), 0, 1);
        f32 t01 = clamp(((max_distance - dist01) / (max_distance - min_distance)), 0, 1);
        f32 t10 = clamp(((max_distance - dist10) / (max_distance - min_distance)), 0, 1);
        f32 t11 = clamp(((max_distance - dist11) / (max_distance - min_distance)), 0, 1);

        f32 tess00 = mix(min_tess_level, max_tess_level, t00);
        f32 tess01 = mix(min_tess_level, max_tess_level, t01);
        f32 tess10 = mix(min_tess_level, max_tess_level, t10);
        f32 tess11 = mix(min_tess_level, max_tess_level, t11);

        gl_TessLevelOuter[0] = tess00;
        gl_TessLevelOuter[1] = tess01;
        gl_TessLevelOuter[2] = tess10;
        gl_TessLevelOuter[3] = tess11;

        gl_TessLevelInner[0] = max(tess01, tess11);
        gl_TessLevelInner[1] = max(tess00, tess10);
    }
}

)";
