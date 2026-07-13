#include "open_simplex.hlsl"

cbuffer SurfaceInfo : register(b0, space3)
{
	// @todo
};

cbuffer CameraInfo : register(b1, space3)
{
    float4 camera;
    float2 offset;
    float2 padding;
};

struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex : TEXCOORD0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
	// @todo
    return color;
}
