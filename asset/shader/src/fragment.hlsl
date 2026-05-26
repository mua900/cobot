cbuffer buffer : register(b0, space3)
{
    float time;
    float3 resolution;
};

void main(float4 fragCoord : SV_Position, out float4 fragColor : SV_Target)
{
    float2 uv = fragCoord.xy / resolution.xy;
    float3 col = 0.5 + 0.5 * cos(time + uv.xyx + float3(0,4,2));
    
    fragColor = float4(1,1,1,1);
    // fragColor = float4(col, 1.0);
}
