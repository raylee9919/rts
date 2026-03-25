// Copyright Seong Woo Lee. All Rights Reserved.

R"(

in VS_Out {
    vec3 fP;
    vec3 fN;
    vec3 fT;
    vec2 fUV;
    vec4 fC;
    float fSign;
} /*instance name (optional)*/;


out vec4 result_color;

uniform vec4 tint;


uniform uvec2 u_albedo;
uniform uvec2 u_normal;
uniform uvec2 u_roughness;
uniform uvec2 u_metallic;
uniform uvec2 u_emission;
layout(binding = 6) uniform sampler2DArray shadowmaps;

layout(std140, row_major, binding = 7) uniform PBR_Uniform_Block 
{
    /*  0*/vec4 wireframe_color;
    /* 16*/vec3 eye_position;
    /* 32*/vec3 to_light;
    /* 48*/mat4 shadowmap_view_projs[CSM_COUNT];
    /*304*/mat4 csm_view;
    /*368*/vec4 csm_z_spans; // instead of 'float csm_z_spans[CSM_COUNT]' for better packing.
    /*384*/
};

vec3 schlick_fresnel(vec3 f0, float ndoth) 
{
    return f0 + (vec3(1.0) - f0) * pow(2.0, (-5.55473 * ndoth - 6.98316) * ndoth);
}

vec3 ndf_ggx(float ndoth, float roughness) 
{
    float alpha = roughness * roughness;
    float alpha_squared = alpha * alpha;
    float denom = (ndoth * ndoth) * (alpha_squared - 1.0) + 1.0;

    vec3 result = vec3(alpha_squared / (3.141592 * denom * denom));
    return result;
}

float schlick_g1(float ndotv, float k) 
{
    return ndotv / (ndotv * (1.0 - k) + k);
}

vec3 schlick_ggx(float ndoti, float ndoto, float roughness) 
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    vec3 result = vec3(schlick_g1(ndoti, k) * schlick_g1(ndoto, k));
    return result;
}

void main()
{
    // @Todo:
    // Bunch of static branches. I assume it isn't as bad as I think, but using 
    // default textures would be nicer.
    //
    if (true) 
    {
        vec3 DEBUG_light_radiance = vec3(4.0);

        vec3 to_eye = normalize(eye_position - fP);

        vec3 albedo     = (u_albedo.x == 0 && u_albedo.y == 0) ? vec3(1.0) : pow(texture(sampler2D(u_albedo), fUV).rgb, vec3(2.2));
        float roughness = (u_roughness.x == 0 && u_roughness.y == 0) ? 1.0 : texture(sampler2D(u_roughness), fUV).r;
        float metallic  = (u_metallic.x == 0 && u_metallic.y == 0) ? 0.0 : texture(sampler2D(u_metallic), fUV).r;
        vec3 emission   = (u_emission.x == 0 && u_emission.y == 0) ? vec3(0.0) : pow(texture(sampler2D(u_emission), fUV).rgb, vec3(2.2));

        vec3 n = (u_normal.x == 0 && u_normal.y == 0) ? vec3(0.0, 0.0, 1.0) : texture(sampler2D(u_normal), fUV).rgb * 2.0 - 1.0;
        vec3 N = normalize(fN);
        vec3 T = normalize(fT);
        vec3 B = normalize(fSign * cross(fN, fT));
        n = T*n.x + B*n.y + N*n.z;
        n = normalize(n);


        vec3 direct_lighting = vec3(0);

        vec3 h = normalize(to_eye + to_light);
        vec3 l = normalize(to_light);
        vec3 v = normalize(to_eye);

        float ndoti = max(0, dot(n, l));
        float ndoth = max(0, dot(n, h));
        float ndoto = max(0, dot(n, v));

        vec3 f_dielectric = vec3(0.04);
        vec3 f0 = mix(f_dielectric, albedo, metallic);
        vec3 f = schlick_fresnel(f0, max(0.0, dot(h, l)));
        vec3 kd = (vec3(1.0) - f) * (1.0 - metallic);
        vec3 diffuse_brdf = kd * albedo;

        vec3 d = ndf_ggx(ndoth, roughness);
        vec3 g = schlick_ggx(ndoti, ndoto, roughness);
        vec3 specular_brdf = (f * d * g) / max(4.0 * ndoti * ndoto, 0.001);

        vec3 radiance = DEBUG_light_radiance;

        direct_lighting += (diffuse_brdf + specular_brdf) * radiance * ndoti;

        result_color = vec4(direct_lighting, 1);


        /* Shadowmap */
        float shadowness = 0.0;
        vec4 view_space_frag = csm_view * vec4(fP, 1);
        float view_space_z = abs(view_space_frag.z);
        int layer = -1;
        for (int i = 0; i < CSM_COUNT; ++i) {
            if (view_space_z <= csm_z_spans[i]) {
                layer = i;
                break;
            }
        }
        if (layer != -1) {
            vec4 frag = shadowmap_view_projs[layer] * vec4(fP, 1);
            frag /= frag.w;
            frag.xyz = frag.xyz * 0.5 + 0.5;
            vec2 shadowmap_uv = frag.xy;
            float bias = max(0.0005, 0.005*(1.0 - ndoti));; // TODO: Study
            const v2 poisson_disk[16] = { // [-1,1]
                v2(-0.942016, -0.399062),  v2( 0.945586, -0.768907),
                v2(-0.0941841,-0.929388),  v2( 0.344959,  0.293877),
                v2(-0.915885,  0.457714),  v2(-0.815442, -0.879124),
                v2(-0.382775,  0.276768),  v2( 0.974843,  0.756483),
                v2( 0.443233, -0.975115),  v2( 0.537429, -0.473734),
                v2(-0.264969, -0.418930),  v2( 0.791975,  0.190901),
                v2(-0.241888,  0.997065),  v2(-0.814099,  0.914375),
                v2( 0.199841,  0.786413),  v2( 0.143831, -0.141007)
            };
            const float inv_possion_radius = 1.0 / 700; // TODO: Study
            for (int i = 0; i < 16; ++i) {
                v2 offset = poisson_disk[i] * inv_possion_radius;
                float depth = texture(shadowmaps, vec3(shadowmap_uv + offset, layer)).r;
                if (frag.z > depth + bias) {
                    shadowness += 0.5;
                }
            }
            const float inv_sample_count = 1.0/16.0;
            shadowness *= inv_sample_count;
        }

        result_color.rgb *= (1.0 - shadowness);
        result_color *= tint;




// CSM Layer debugging
#if 0
        vec3 colors[] = {
            vec3(1,0,0),
            vec3(0,1,0),
            vec3(0,0,1),
        };
        result_color.rgb = colors[layer % 3];

#endif
    } else {
        result_color = wireframe_color;
    }
}

)";
