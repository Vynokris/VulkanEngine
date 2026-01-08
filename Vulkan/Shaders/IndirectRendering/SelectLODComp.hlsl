[[vk::binding(0, 0)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(0, 1)]] row_major StructuredBuffer<float4x4> modelMatrices;
[[vk::binding(1, 1)]] RWStructuredBuffer<uint> selectedSections;

[[vk::push_constant]] cbuffer pushConstants
{
    row_major float4x4 viewProj;
    float3 viewPos;
};

[numthreads(128, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint instanceID = threadID.x;
    if (instanceID >= totalInstanceCount)
        return;

    const float3 instancePos = modelMatrices[instanceID][3].xyz;
    const float  camDist = distance(viewPos, instancePos);

    // Temporary
    const float farPlaneDist = 100.f;
    const float camDist01 = saturate(camDist / farPlaneDist);
    const uint  selectedSection = uint(floor(camDist01 * (float(sectionCount) - 1e-5f)));

    selectedSections[instanceID] = selectedSection;
}
