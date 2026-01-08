[[vk::binding(0, 0)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(1, 1)]] StructuredBuffer<uint>   selectedSections;
[[vk::binding(0, 2)]] RWStructuredBuffer<uint> drawIndirect;

#define DRAW_INDIRECT_ARGS_COUNT 5

[numthreads(128, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint instanceID = threadID.x;
    if (instanceID >= totalInstanceCount)
        return;

    const uint selectedSection = selectedSections[instanceID];
    const uint writeOffset = DRAW_INDIRECT_ARGS_COUNT * selectedSection + 1;
    
    uint dummyOutput;
    InterlockedAdd(drawIndirect[writeOffset], 1, dummyOutput);
}
