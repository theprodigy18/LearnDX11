struct PSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D hdrTexture : register(t0);

SamplerState linearSampler : register(s0);

float4 PSMain(PSInput input) : SV_Target
{
    float3 rgb       = hdrTexture.Sample(linearSampler, input.uv).rgb;
    float  intensity = max(rgb.x, max(rgb.y, rgb.z));

    if (intensity > 1.0)
        return float4(rgb, 1.0);

    return float4(0.0, 0.0, 0.0, 0.0);
}