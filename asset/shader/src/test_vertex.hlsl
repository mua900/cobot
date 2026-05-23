struct VSInput {
    float3 position : POSITION;
};

float4 main(VSInput input) : SV_POSITION
{
    return float4(input.position, 1.0);
}
