[[vk::binding(3, 1)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(0, 0)]] RWStructuredBuffer<uint> drawIndirect;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> indirectionOffsets;

#define DRAW_INDIRECT_ARGS_COUNT 5

[numthreads(8, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint sectionID = threadID.x;
    if (sectionID >= sectionCount)
        return;

    const uint drawIndirectStartOffset  = DRAW_INDIRECT_ARGS_COUNT * sectionID;
    const uint instanceCountWriteOffset = drawIndirectStartOffset + 1;
    const uint firstInstanceWriteOffset = drawIndirectStartOffset + 4;
    drawIndirect[instanceCountWriteOffset] = 0;
    drawIndirect[firstInstanceWriteOffset] = 0;
    indirectionOffsets[sectionID] = 0;
}
