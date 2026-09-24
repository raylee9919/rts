// Copyright Seong Woo Lee. All Rights Reserved.

module material;

// Define 'Texture' attribute
[__AttributeUsage(_AttributeTargets.Var)]
public struct TextureAttribute {};

public struct Material_Input {
    public float3   position;
    public float3   normal;
    public float4   tangent; // w: bitangent sign
    public float2   uv;
    public uint32_t linear_sampler_id;
};

public interface IMaterial 
{
    float4 eval( Material_Input input, float3 radiance, float3 wi, float3 wo );
};


//
// Cook-Torrance BRDF
// @Todo: Better approximation?
//
public float3 schlick_fresnel(float3 f0, float hdotl)
{
    return f0 + (1.0 - f0) * pow(2.0, (-5.55473 * hdotl - 6.98316) * hdotl);
}

public float ndf_ggx(float ndoth, float roughness)
{
    float alpha         = roughness * roughness;
    float alpha_squared = alpha * alpha;
    float denom         = (ndoth * ndoth) * (alpha_squared - 1.0) + 1.0;
    return alpha_squared / (3.141592 * denom * denom);
}

public float schlick_g1(float ndotv, float k)
{
    return ndotv / (ndotv * (1.0 - k) + k);
}

public float schlick_ggx(float ndoti, float ndoto, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return schlick_g1(ndoti, k) * schlick_g1(ndoto, k);
}

public float3 brdf_cook_torrance(float3 n, float3 wi, float3 wo, float3 albedo, float roughness, float metallic)
{
    float3 h = normalize(wi + wo);

    float ndoti = max(0.0, dot(n, wi));
    float ndoth = max(0.0, dot(n, h));
    float ndoto = max(0.0, dot(n, wo));

    float3 f0 = lerp(float3(0.04), albedo, metallic);
    float3 f  = schlick_fresnel(f0, max(0.0, dot(h, wi)));
    float3 kd = (1.0 - f) * (1.0 - metallic);

    float3 diffuse_brdf  = kd * albedo;
    float3 specular_brdf = (f * ndf_ggx(ndoth, roughness) * schlick_ggx(ndoti, ndoto, roughness)) / max(4.0 * ndoti * ndoto, 0.001);

    return (diffuse_brdf + specular_brdf) * ndoti;
}
