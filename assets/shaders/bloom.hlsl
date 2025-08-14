struct PSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

cbuffer BloomParams : register(b0)
{
    float4 weights; // Check up to 4 neighbour pixel.
    float2 texelSize;
    float  weightCenter;
    int    horizontal;
};

Texture2D hdrTexture : register(t0);

SamplerState linearSampler : register(s0);

float4 PSMain(PSInput input) : SV_Target
{
    // Test sampling di koordinat yang pasti ada content
    float3 color = hdrTexture.Sample(linearSampler, input.uv).rgb;
    return float4(color, 1.0);

    // return hdrTexture.Sample(linearSampler, input.uv);
    // float4 result = color * weightCenter;

    // float2 neighbourUV = horizontal ? float2(texelSize.x, 0) : float2(0, texelSize.y);

    // for (uint i = 1; i < 5; ++i)
    // {
    //     result += hdrTexture.Sample(linearSampler, input.uv + neighbourUV * i) * weights[i - 1];
    //     result += hdrTexture.Sample(linearSampler, input.uv - neighbourUV * i) * weights[i - 1];
    // }

    // return float4(1.0, 0.0, 0.0, 1.0);
}