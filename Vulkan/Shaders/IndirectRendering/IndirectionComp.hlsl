// Const data input.
[[vk::binding(0, 0)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(1, 1)]] StructuredBuffer<uint>   selectedSections;
[[vk::binding(1, 2)]] StructuredBuffer<uint>   indirectionOffsets;
[[vk::binding(2, 1)]] RWStructuredBuffer<uint> indirection;

[numthreads(128, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint instanceID = threadID.x;
    if (instanceID >= totalInstanceCount)
        return;

    const uint selectedSection = selectedSections[instanceID];
    
    uint outIndex;
    InterlockedAdd(indirectionOffsets[selectedSection], 1, outIndex);
    indirection[outIndex] = instanceID;
}
