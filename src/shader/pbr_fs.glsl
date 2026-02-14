// Copyright Seong Woo Lee. All Rights Reserved.

R"(

in vec3 fP;
in vec3 fN;
in v2 fUV;
in vec4 fC;
in m3x3 TBN;

layout(location=0) out vec4 result_color;

uniform u32 flags;
uniform vec4 wireframe_color;
uniform vec4 tint;
uniform vec3 eye_position;
uniform m4x4 shadowmap_view_projs[CSM_COUNT];
uniform vec3 to_light;

uniform m4x4 csm_view;
uniform float csm_z_spans[CSM_COUNT];

layout(binding=0) uniform sampler2D albedo_texture;
layout(binding=1) uniform sampler2D normal_texture;
layout(binding=2) uniform sampler2D roughness_texture;
layout(binding=3) uniform sampler2D metalic_texture;
layout(binding=4) uniform sampler2D emission_texture;
layout(binding=5) uniform sampler2D orm_texture;
layout(binding=6) uniform sampler2DArray shadowmaps;


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
    //vec3 result = vec3(r);
    vec3 result = vec3(schlick_g1(ndoti, k) * schlick_g1(ndoto, k));
    return result;
}

void main()
{
    if ((flags & Pbr_No_Lighting) == 0) 
    {
        vec3 DEBUG_light_radiance = vec3(4.f, 4.f, 4.f);

        vec3 to_eye = normalize(eye_position - fP);

        vec3 albedo;
        if ((flags & Pbr_Has_albedo) != 0) {
            albedo = pow(texture(albedo_texture, fUV).rgb, vec3(2.2));
        } else {
            albedo = pow(fC.rgb, vec3(2.2));
        }

        vec3 normal;
        if ((flags & Pbr_Has_normal) != 0) {
            normal = texture(normal_texture, fUV).rgb * 2.0 - 1.0;
            normal = normalize(TBN * normal);
        } else {
            normal = fN;
        }

        float metalic;
        if ((flags & Pbr_Has_roughness) != 0) {
            metalic = texture(metalic_texture, fUV).r;
        } else {
            metalic = 0.0;
        }

        float roughness;
        if ((flags & Pbr_Has_metalic) != 0) {
            roughness = texture(roughness_texture, fUV).r;
        } else {
            roughness = 1.0;
        }

        vec3 emission;
        if ((flags & Pbr_Has_emission) != 0) {
            emission = pow(texture(emission_texture, fUV).rgb, vec3(2.2));
        } else {
            emission = vec3(0);
        }

        vec3 direct_lighting = vec3(0);

        vec3 halfway = normalize(to_eye + to_light);

        float ndoti = max(0, dot(normal, to_light));
        float ndoth = max(0, dot(normal, halfway));
        float ndoto = max(0, dot(normal, to_eye));

        vec3 f_dielectric = vec3(0.04);
        vec3 f0 = mix(f_dielectric, albedo, metalic);
        vec3 f = schlick_fresnel(f0, max(0.0, dot(halfway, to_eye)));
        vec3 kd = mix(vec3(1) - f, vec3(0,0,0), metalic);
        vec3 diffuse_brdf = kd * albedo;

        vec3 d = ndf_ggx(ndoth, roughness);
        vec3 g = schlick_ggx(ndoti, ndoto, roughness);
        vec3 specular_brdf = (f * d * g) / max(1e-5, 4.0 * ndoti * ndoto);

        vec3 radiance = DEBUG_light_radiance;

        direct_lighting += (diffuse_brdf + specular_brdf) * radiance * ndoti;

        result_color = vec4(direct_lighting, 1);


        /* Shadowmap */
        float shadowness = 0.0;
        vec4 view_space_frag = csm_view * vec4(fP, 1);
        float view_space_z = abs(view_space_frag.z);
        s32 layer = -1;
        for (s32 i = 0; i < CSM_COUNT; ++i) {
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
            for (s32 i = 0; i < 16; ++i) {
                v2 offset = poisson_disk[i] * inv_possion_radius;
                float depth = texture(shadowmaps, vec3(shadowmap_uv + offset, layer)).r;
                if (frag.z > depth + bias) {
                    shadowness += 0.8;
                }
            }
            const float inv_sample_count = 1.0/16.0;
            shadowness *= inv_sample_count;
        }

        result_color.rgb *= (1.0f - shadowness);
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
