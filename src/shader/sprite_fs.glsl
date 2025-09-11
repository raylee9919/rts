R"(

uniform v4 color;
uniform sampler2D texture_sample;

in v2   fUV;
out v4  C;

void main()
{
    // sRGB
    v4 const_color = color;
    const_color.rgb = pow(const_color.rgb, v3(2.2));

    // sRGB
    v4 texture_color = texture(texture_sample, fUV);
    texture_color.rgb = pow(texture_color.rgb, v3(2.2));


    C = const_color * texture_color;

    // sRGB
    if (C.a == 0.0f) {
        discard;
    } else {
        f32 k = 1/2.2;
        C.rgb = pow(C.rgb, v3(k));
    }
}

)";
