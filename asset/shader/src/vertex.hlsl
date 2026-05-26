cbuffer buffer : register(b0, space1)
{
    float4x4 ModelViewProjection;
};

struct VSInput {
    float3 position : POSITION;
};

float4 main(VSInput input) : SV_POSITION
{
    float4 pos = float4(input.position, 1.0);
    // return mul(ModelViewProjection, pos);
    return pos;
}
