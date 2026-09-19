// Copyright Seong Woo Lee. All Rights Reserved.

module M_interface;

interface IMaterial
{
    float4 eval( float3 wi, float3 wo );
}
