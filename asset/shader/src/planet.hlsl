#include "open_simplex.hlsl"

cbuffer BodyInfo : register(b0, space3)
{
    float3 position;
    float t;
};

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    return float4(position, 1.0);
}
