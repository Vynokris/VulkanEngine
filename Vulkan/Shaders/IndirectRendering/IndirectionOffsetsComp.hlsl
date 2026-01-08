[[vk::binding(3, 1)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(0, 0)]] StructuredBuffer<uint>   drawIndirect;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> indirectionOffsets;

#define DRAW_INDIRECT_ARGS_COUNT 5

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint sectionID       = threadID.x;
    const uint targetSectionID = threadID.y + 1;
    if (sectionID >= sectionCount - 1 || targetSectionID >= sectionCount || targetSectionID <= sectionID)
        return;

    const uint readIndex = DRAW_INDIRECT_ARGS_COUNT * sectionID + 1;
    const uint subSectionInstanceCount = drawIndirect[readIndex];
    const uint writeIndex = DRAW_INDIRECT_ARGS_COUNT * targetSectionID + 4;
    
    uint dummyOutput;
	InterlockedAdd(drawIndirect      [writeIndex],      subSectionInstanceCount, dummyOutput);
    InterlockedAdd(indirectionOffsets[targetSectionID], subSectionInstanceCount, dummyOutput);
}
