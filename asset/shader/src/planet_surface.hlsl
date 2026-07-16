#include "utility.hlsl"
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

float sdCircle(float3 p, float3 center, float rad)
{
    return length(p - center) - rad;
}

float map(float3 p)
{
    float d = sdCircle(p, float3(100, 100, 1), 100);

    return d;
}

float3 calc_normal(float3 pos)
{
    float3 e = float3(0.01,0.,0.);
    return normalize( float3( map(pos+e.xyy)-map(pos-e.xyy),
                              map(pos+e.yxy)-map(pos-e.yxy),
                              map(pos+e.yyx)-map(pos-e.yyx)));
}

float4 raymarch(float3 ro, float3 rd)
{
    float3 pos = float3(0,0,0);
    float t = 0;
    for (int i = 0; i < 16; i++)
    {
        pos = ro + rd * t;
        float d = map(pos);

        if (d < 0.01)
        {
            break;
        }

        t += d;

        if (t >= 20)
        {
            break;
        }
    }

    return float4(t, 0, 0, 0);
}

float4 fbm(float3 x, int numOctaves)
{
    const float G = exp2(0.5);
    float f = 1;
    float a = 1;
    float4 t = 0;
    for (int i = 0; i < numOctaves; i++)
    {
        t += a * openSimplex2_Conventional(x * f);
        f *= 2;
        a *= G;
    }

    return t;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 p = float3(screenToWorld(input.pos.xy, camera, offset), input.pos.z);
    float3 np = normalize(p);
    float4 d = raymarch(p, np);

    float4 n = fbm(p + fbm(p + fbm(p, 6).xyz, 4).xyz, 10);
    float3 col = lerp(n.xyz, input.color.xyz, 0.5);
    col = lerp(col, d.xxx, 0.5);
    return float4(col, 1);
}
