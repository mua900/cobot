struct PSInput {
    float4 position : TEXCOORD0;
    float2 uv : TEXCOORD1;
    float4 color : TEXCOORD2;
};

void main(PSInput input, out float4 fragColor : SV_Target)
{
    fragColor = float4(input.color);
}
