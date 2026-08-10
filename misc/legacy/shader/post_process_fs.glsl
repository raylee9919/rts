// Copyright Seong Woo Lee. All Rights Reserved.

R"(

uniform sampler2D scene_tex;

in vec2 uv;

out vec4 result;

vec3 tone_mapping_reinhard(vec3 c) {
    return c / (1.0 + c);
}

vec3 tone_mapping_jodie_reinhard(vec3 c) {
    // https://www.shadertoy.com/view/tdSXzD
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    vec3 tc = c / (c + 1.0);
    return mix(c / (luma + 1.0), tc, tc);
}

vec3 srgb_from_linear(vec3 c) {
    return vec3(c.r <= 0.0031308 ? c.r * 12.92 : 1.055 * pow(c.r, 1.0 / 2.4) - 0.055,
                c.g <= 0.0031308 ? c.g * 12.92 : 1.055 * pow(c.g, 1.0 / 2.4) - 0.055,
                c.b <= 0.0031308 ? c.b * 12.92 : 1.055 * pow(c.b, 1.0 / 2.4) - 0.055);
}

void main()
{
    vec4 texel = texture(scene_tex, uv);
    vec3 color = texel.rgb;

    color = tone_mapping_reinhard(color);

    color = srgb_from_linear(color);

    result = vec4(color, texel.a);
}

)";
