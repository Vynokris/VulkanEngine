[[vk::binding(0, 0)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(0, 2)]] StructuredBuffer<uint>   drawIndirect;
[[vk::binding(1, 2)]] RWStructuredBuffer<uint> indirectionOffsets;

#define DRAW_INDIRECT_ARGS_COUNT 5

[numthreads(8, 8, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint sectionID       = threadID.x;
    const uint targetSectionID = threadID.y;
    if (sectionID >= sectionCount - 1 || targetSectionID >= sectionCount || targetSectionID <= sectionID)
        return;

    const uint readIndex = DRAW_INDIRECT_ARGS_COUNT * sectionID + 1;
    const uint subSectionInstanceCount = drawIndirect[readIndex];
    
    uint dummyOutput;
    InterlockedAdd(indirectionOffsets[targetSectionID],                subSectionInstanceCount, dummyOutput);
	InterlockedAdd(indirectionOffsets[targetSectionID + sectionCount], subSectionInstanceCount, dummyOutput);
}
