// Copyright Seong Woo Lee. All Rights Reserved.

module material;

[__AttributeUsage(_AttributeTargets.Var)]
public struct TextureAttribute {};

public struct Material_Input {
    public float3   position;
    public float3   normal;
    public float2   uv;
    public uint32_t linear_sampler_id;
};

public interface IMaterial {
    float4 eval( Material_Input input, float3 wi, float3 wo );
};
