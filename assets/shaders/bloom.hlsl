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
    float4 result = hdrTexture.Sample(linearSampler, input.uv) * weightCenter;

    float2 neighbourUV = horizontal ? float2(texelSize.x, 0) : float2(0, texelSize.y);

    for (uint i = 1; i < 5; ++i)
    {
        result += hdrTexture.Sample(linearSampler, input.uv + neighbourUV * i) * weights[i - 1];
        result += hdrTexture.Sample(linearSampler, input.uv - neighbourUV * i) * weights[i - 1];
    }

    return result;
}