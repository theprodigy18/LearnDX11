struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput VSMain(uint id : SV_VertexID)
{
    VSOutput output;
    output.uv    = float2((id << 1) & 2, id & 2);
    output.pos   = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    output.pos.y = -output.pos.y;

    return output;
}

struct PSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D hdrTexture : register(t0);

SamplerState linearSampler : register(s0);

float4 PSMain(PSInput input) : SV_Target
{
    return hdrTexture.Sample(linearSampler, input.uv);
}