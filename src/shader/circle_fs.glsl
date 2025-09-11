R"(

in v3 fP;
in flat v3 center;

out v4 result;

uniform f32 radius;

void main()
{
    f32 d = distance(center, fP);
    f32 thickness = 0.05f;
    if (d > radius) discard;
    if (d < radius - thickness) discard;
    result = v4(0.6f, 1.0f, 0.5f, 0.4f);
}

)";
