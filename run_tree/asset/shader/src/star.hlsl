#include "open_simplex.hlsl"
#include "utility.hlsl"

cbuffer Buff : register(b1, space3)
{
    float4 camera;
    float2 offset;
    float starRadius;
    float padding;
};

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float2 fragment = screenToWorld(input.pos.xy, camera, offset);

    float2 delta = fragment;
    float d2 = dot(delta,delta);
    float r2 = starRadius * starRadius;

    // clip(r2 - d2);

    float dz = sqrt(r2 - d2);

    float3 surfacePosition = float3(fragment, dz);

    float3 normal = float3(delta, dz) / starRadius;

    float4 n = openSimplex2_Conventional(surfacePosition);

    return input.color;
}
