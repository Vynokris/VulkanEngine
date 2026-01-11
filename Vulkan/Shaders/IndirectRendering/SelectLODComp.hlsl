[[vk::binding(3, 1)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(2, 0)]] StructuredBuffer<float> lodDistances;
[[vk::binding(0, 1)]] row_major StructuredBuffer<float4x4> modelMatrices;
[[vk::binding(1, 1)]] RWStructuredBuffer<uint> selectedSections;

[[vk::push_constant]] cbuffer pushConstants
{
    row_major float4x4 viewProj;
    float3 viewPos;
};

#define CULL_BIAS_L .5f
#define CULL_BIAS_R .5f
#define CULL_BIAS_B 1.f
#define CULL_BIAS_T 1.f
#define CULL_BIAS_N 0.f
#define CULL_BIAS_F 0.f

[numthreads(128, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint instanceID = threadID.x;
    if (instanceID >= totalInstanceCount)
        return;

    // Frustum culling (check only instance position because I don't have bounding boxes)
    const float4 instanceClip = (viewProj * modelMatrices[instanceID])[3];
    if (!(-instanceClip.w - CULL_BIAS_L <= instanceClip.x &&
          -instanceClip.w - CULL_BIAS_B <= instanceClip.y &&
          -instanceClip.w - CULL_BIAS_N <= instanceClip.z &&
           instanceClip.x <= instanceClip.w + CULL_BIAS_R &&
           instanceClip.y <= instanceClip.w + CULL_BIAS_T &&
           instanceClip.z <= instanceClip.w + CULL_BIAS_F))
    {
        selectedSections[instanceID] = -1u;
        return;
    }

    // LOD selection
    const float3 instancePos = modelMatrices[instanceID][3].xyz;
    const float camDist = distance(viewPos, instancePos);
    uint selectedSection = sectionCount - 1;
    for (uint i = 0; i < 8; i++)
    {
        if (i >= sectionCount - 1 || camDist <= lodDistances[i])
        {
            selectedSection = i;
            break;
        }
    }
    selectedSections[instanceID] = selectedSection;
}
