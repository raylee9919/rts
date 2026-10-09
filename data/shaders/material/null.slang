// Copyright Seong Woo Lee. All Rights Reserved.

/*
    Material with NULL ID will fallback to this shader.
*/

module material_default;

import basic;
import material;
#include "shared/shared.h"

struct M_Default : IMaterial
{
    /* Evaluation Implementation */
    float4 eval( Material_Input input, float3 radiance, float3 wi, float3 wo )
    {
        float3 result = radiance * dot(input.normal, wi);
        return float4(result, 1.0);
    }
};

export struct Material : IMaterial = M_Default;
