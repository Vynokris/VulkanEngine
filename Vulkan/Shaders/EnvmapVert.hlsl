// Outputs to the fragment shader.
struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(0)]] float3 vertPos : POSITION;
};

// Push constants input.
[[vk::push_constant]] cbuffer pushConstants
{
    row_major float4x4 viewProj;
};

// Constant hardcoded billboard data.
static const float2 vertices[4] =
{
    float2(-1.0,  1.0),
    float2( 1.0,  1.0),
    float2(-1.0, -1.0),
    float2( 1.0, -1.0),
};

// Vertex shader main function
VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output = (VSOutput)0;
    
    const float2 positionNDC = vertices[vertexID];
    const float4 position = float4(positionNDC, 1, 1);
    output.position = position;
    output.vertPos = viewProj * position;
    
    return output;
}
