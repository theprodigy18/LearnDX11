float4 VSMain(uint id : SV_VertexID) : SV_Position
{
    float2 vertices[] = {
        {0.0, 0.5},
        {0.5, -0.5},
        {-0.5, -0.5}};

    return float4(vertices[id], 0.0, 1.0);
}

float4 PSMain() : SV_Target
{
    return float4(2.0, 1.0, 0.0, 1.0);
}