/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Seong Woo Lee $
   $Notice: (C) Copyright %s by Seong Woo Lee. All Rights Reserved. $
   ======================================================================== */

R"(

in v3 fP;
in v3 fN;
in v2 fUV;
in v4 fC;
in m3x3 TBN;

layout(location=0) out v4 result_color;
layout(location=1) out u32 result_id;

uniform u32 entity_id;
uniform u32 hot_entity_id;
uniform u32 active_entity_id;

uniform f32 time;
uniform u32 flags;
uniform v4 wireframe_color;
uniform v4 tint;
uniform v3 eye_position;
uniform m4x4 shadowmap_view_projs[CSM_COUNT];
uniform v3 to_light;

uniform m4x4 csm_view;
uniform f32 csm_z_spans[CSM_COUNT];

layout(binding=0) uniform sampler2D albedo_texture;
layout(binding=1) uniform sampler2D normal_texture;
layout(binding=2) uniform sampler2D roughness_texture;
layout(binding=3) uniform sampler2D metalic_texture;
layout(binding=4) uniform sampler2D emission_texture;
layout(binding=5) uniform sampler2D orm_texture;
layout(binding=6) uniform sampler2DArray shadowmaps;


v3 schlick_fresnel(v3 f0, f32 ndoth) {
    return f0 + (v3(1.0) - f0) * pow(2.0, (-5.55473 * ndoth - 6.98316) * ndoth);
}

v3 ndf_ggx(f32 ndoth, f32 roughness) {
    f32 alpha = roughness * roughness;
    f32 alpha_squared = alpha * alpha;
    f32 denom = (ndoth * ndoth) * (alpha_squared - 1.0) + 1.0;

    v3 result = v3(alpha_squared / (3.141592 * denom * denom));
    return result;
}

f32 schlick_g1(f32 ndotv, f32 k) {
    return ndotv / (ndotv * (1.0 - k) + k);
}

v3 schlick_ggx(f32 ndoti, f32 ndoto, f32 roughness) {
    f32 r = roughness + 1.0;
    f32 k = (r * r) / 8.0;
    //v3 result = v3(r);
    v3 result = v3(schlick_g1(ndoti, k) * schlick_g1(ndoto, k));
    return result;
}

void main()
{
    if ((flags & Pbr_No_Lighting) == 0) 
    {
        v3 DEBUG_light_radiance = v3(3.0);

        v3 to_eye = normalize(eye_position - fP);

        v3 albedo;
        if ((flags & Pbr_Has_albedo) != 0) {
            albedo = pow(texture(albedo_texture, fUV).rgb, v3(2.2));
        } else {
            albedo = pow(fC.rgb, v3(2.2));
        }

        v3 normal;
        if ((flags & Pbr_Has_normal) != 0) {
            normal = texture(normal_texture, fUV).rgb * 2.0 - 1.0;
            normal = normalize(TBN * normal);
        } else {
            normal = fN;
        }

        f32 metalic;
        if ((flags & Pbr_Has_roughness) != 0) {
            metalic = texture(metalic_texture, fUV).r;
        } else {
            metalic = 0.0;
        }

        f32 roughness;
        if ((flags & Pbr_Has_metalic) != 0) {
            roughness = texture(roughness_texture, fUV).r;
        } else {
            roughness = 1.0;
        }

        v3 emission;
        if ((flags & Pbr_Has_emission) != 0) {
            emission = pow(texture(emission_texture, fUV).rgb, v3(2.2));
        } else {
            emission = v3(0);
        }

        v3 direct_lighting = v3(0);

        v3 halfway = normalize(to_eye + to_light);

        f32 ndoti = max(0, dot(normal, to_light));
        f32 ndoth = max(0, dot(normal, halfway));
        f32 ndoto = max(0, dot(normal, to_eye));

        v3 f_dielectric = v3(0.04);
        v3 f0 = mix(f_dielectric, albedo, metalic);
        v3 f = schlick_fresnel(f0, max(0.0, dot(halfway, to_eye)));
        v3 kd = mix(v3(1) - f, v3(0,0,0), metalic);
        v3 diffuse_brdf = kd * albedo;

        v3 d = ndf_ggx(ndoth, roughness);
        v3 g = schlick_ggx(ndoti, ndoto, roughness);
        v3 specular_brdf = (f * d * g) / max(1e-5, 4.0 * ndoti * ndoto);

        v3 radiance = DEBUG_light_radiance;

        direct_lighting += (diffuse_brdf + specular_brdf) * radiance * ndoti;

        result_color = v4(direct_lighting, 1);


        /* Shadowmap */
        v4 view_space_frag = csm_view * v4(fP, 1);
        f32 view_space_z = abs(view_space_frag.z);
        s32 layer = -1;
        for (s32 i = 0; i < CSM_COUNT; ++i) {
            if (view_space_z < csm_z_spans[i]) {
                layer = i;
                break;
            }
        }
        if (layer == -1) {
            layer = CSM_COUNT - 1;
        }
        v4 frag = shadowmap_view_projs[layer] * v4(fP, 1);
        frag /= frag.w;
        frag.xyz = frag.xyz * 0.5 + 0.5;
        v2 shadowmap_uv = frag.xy;
        f32 shadowness = 0.0;
        f32 bias = (1 - ndoto) * 0.005;
        const v2 poisson_disk[16] = { // [-1,1]
            v2(-0.942016, -0.399062),   v2(0.945586, -0.768907),
            v2(-0.0941841, -0.929388),  v2(0.344959, 0.293877),
            v2(-0.915885, 0.457714),    v2(-0.815442, -0.879124),
            v2(-0.382775, 0.276768),    v2(0.974843, 0.756483),
            v2(0.443233, -0.975115),    v2(0.537429, -0.473734),
            v2(-0.264969, -0.418930),   v2(0.791975, 0.190901),
            v2(-0.241888, 0.997065),    v2(-0.814099, 0.914375),
            v2(0.199841, 0.786413),     v2(0.143831, -0.141007)
        };
        const f32 inv_possion_radius = 1.0 / 700;
        for (s32 i = 0; i < 16; ++i) {
            v2 offset = poisson_disk[i] * inv_possion_radius;
            f32 depth = texture(shadowmaps, v3(shadowmap_uv + offset, layer)).r;
            if (frag.z > depth + bias) {
                shadowness += 1.0;
            }
        }
        const f32 inv_sample_count = 1.0/17.0;
        shadowness *= inv_sample_count;

        result_color.rgb *= (1.0f - shadowness);
        result_color *= tint;

// CSM Layer debugging
#if 0
        v3 colors[] = {
            v3(1,0,0),
            v3(0,1,0),
            v3(0,0,1),
        };
        result_color.rgb = colors[layer % 3];
#endif
        result_id = entity_id;
    } else {
        result_color = wireframe_color;
    }
}

)";
