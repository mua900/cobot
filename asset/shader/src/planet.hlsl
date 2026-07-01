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

    float2 delta = fragment - position.xy;
    float d2 = dot(delta,delta);
    float r2 = radius * radius;

    clip(r2 - d2);

    float dz = sqrt(r2 - d2);

    float3 surfacePosition = float3(fragment, position.z + dz);

    float3 normal = float3(delta, dz) / radius;

    float3 lightDir = normalize(-surfacePosition);
    float ndotl = saturate(dot(normal, lightDir));

    float3 albedo = input.color.rgb;
    float3 ambient = albedo * 0.08;
    float3 diffuse = albedo * ndotl * 0.8;

    float3 halfVec = normalize(lightDir + float3(0,0,1));
    float specular = pow(saturate(dot(normal, halfVec)), 16.0) * 0.12;

    float maxDepth = 10.0;
    float z = smoothstep(-maxDepth / 2, maxDepth / 2, position.z);
    // remap to 0.5 - 1.0 range
    z = (z + 1.0f) / 2;

    return float4(diffuse + ambient + specular,z);
}
