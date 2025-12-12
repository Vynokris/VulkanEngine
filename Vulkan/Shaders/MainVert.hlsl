// Vertex data inputs.
struct VSInput
{
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float2 texCoord : TEXCOORD;
    [[vk::location(2)]] float3 normal   : NORMAL;
    [[vk::location(3)]] float3 tangent  : TANGENT;
    [[vk::location(4)]] float3 binormal : BINORMAL;
};

// Outputs to the fragment shader.
struct VSOutput
{
    float4 position : SV_POSITION;
    [[vk::location(0)]] float3   fragPos   : POSITION;
    [[vk::location(1)]] float2   texCoord  : TEXCOORD;
    [[vk::location(2)]] float3   normal    : NORMAL;
    [[vk::location(3)]] float3x3 tbnMatrix : NORMAL1; // This variable uses 3 locations in total.
};

// Model, view and projection matrices input.
struct ModelMatrices
{
    float4x4 model;
    float4x4 mvp;
};
[[vk::binding(0, 0)]] row_major ConstantBuffer<ModelMatrices> matrices;

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput)0;
    
    output.position = matrices.mvp   * float4(input.position, 1);
    output.fragPos  = matrices.model * float4(input.position, 1);
    output.texCoord = input.texCoord;
    
    output.normal    = normalize((matrices.model * float4(input.normal,   0)).xyz);
    float3 tangent   = normalize((matrices.model * float4(input.tangent,  0)).xyz);
    float3 binormal  = normalize((matrices.model * float4(input.binormal, 0)).xyz);
    output.tbnMatrix = float3x3(tangent, binormal, output.normal);
    
    return output;
}
