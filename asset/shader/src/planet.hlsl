#include "open_simplex.hlsl"
#include "utility.hlsl"

cbuffer BodyInfo : register(b0, space3)
{
    float3 position;
    float radius;
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
    float2 tex : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PixelShaderInput input) : SV_TARGET
{
    float2 fragment = screenToWorld(input.pos.xy, camera, offset);

    float maxDepth = 10.0;
    float z = smoothstep(-maxDepth / 2, maxDepth / 2, position.z);
    // remap to 0.5 - 1.0 range
    z = (z + 1.0f) / 2;

    return float4(camera.x,0,0,z);
}
