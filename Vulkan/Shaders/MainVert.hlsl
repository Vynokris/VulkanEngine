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
    [[vk::location(2)]] float3x3 tbnMatrix : NORMAL; // This variable uses 3 locations in total.
};

// Push constants input.
[[vk::push_constant]] cbuffer pushConstants
{
    row_major float4x4 viewProj;
    float3 viewPos;
};

// Model matrices input.
[[vk::binding(0, 0)]] row_major StructuredBuffer<float4x4> modelMatrices;

// Vertex shader main function
VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output = (VSOutput)0;
    const float4x4 modelMat = modelMatrices[instanceID];
    
    output.fragPos  = modelMat * float4(input.position, 1);
    output.position = viewProj * float4(output.fragPos, 1);
    output.texCoord = input.texCoord;
    
    const float3 normal   = normalize((modelMat * float4(input.normal,   0)).xyz);
    const float3 tangent  = normalize((modelMat * float4(input.tangent,  0)).xyz);
    const float3 binormal = normalize((modelMat * float4(input.binormal, 0)).xyz);
    output.tbnMatrix = float3x3(tangent, binormal, normal);
    
    return output;
}
