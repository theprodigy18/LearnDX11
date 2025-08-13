struct VSInput
{
    float2 pos : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.pos   = float4(input.pos, 0.0, 1.0);
    output.color = input.color;

    return output;
}

struct PSInput
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

float4 PSMain(PSInput input) : SV_Target
{
    return input.color;
}