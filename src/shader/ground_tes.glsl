R"(
layout (quads, equal_spacing, ccw) in;

uniform sampler2D heightmap;
uniform m4x4 view_proj;
uniform f32 elevation;

in v3 eP[];
in v3 eN[];
in v2 eUV[];
in v4 eC[];
in m3x3 eTBN[];

out v3 fP;
out v3 fN;
out v2 fUV;
out v4 fC;
out m3x3 TBN;

void main()
{
    f32 u = gl_TessCoord.x;
    f32 v = gl_TessCoord.y;

    v3 n00 = eN[0];
    v3 n01 = eN[1];
    v3 n10 = eN[2];
    v3 n11 = eN[3];
    v3 n0  = mix(n00, n01, u);
    v3 n1  = mix(n10, n11, u);
    v3 n   = mix(n0, n1, v);

    v2 uv00 = eUV[0];
    v2 uv01 = eUV[1];
    v2 uv10 = eUV[2];
    v2 uv11 = eUV[3];
    v2 uv0  = mix(uv00, uv01, u);
    v2 uv1  = mix(uv10, uv11, u);
    v2 uv   = mix(uv0, uv1, v);

    v3 p00 = eP[0];
    v3 p01 = eP[1];
    v3 p10 = eP[2];
    v3 p11 = eP[3];
    v3 p0  = mix(p00, p01, u);
    v3 p1  = mix(p10, p11, u);
    v3 p   = mix(p0, p1, v);

    v4 c00 = eC[0];
    v4 c01 = eC[1];
    v4 c10 = eC[2];
    v4 c11 = eC[3];
    v4 c0  = mix(c00, c01, u);
    v4 c1  = mix(c10, c11, u);
    v4 c   = mix(c0, c1, v);

    f32 height01 = texture(heightmap, uv).r;
    p.y += (elevation * height01);

    fP  = p;
    fN  = n;
    fUV = uv;
    fC  = c;
    TBN = eTBN[0];

    gl_Position = view_proj * v4(p, 1);
}

)";
