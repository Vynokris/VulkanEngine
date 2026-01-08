[[vk::binding(0, 0)]] cbuffer constData
{
    uint totalInstanceCount;
    uint sectionCount;
};

[[vk::binding(0, 2)]] RWStructuredBuffer<uint> drawIndirect;

#define DRAW_INDIRECT_ARGS_COUNT 5

[numthreads(8, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID)
{
    const uint sectionID = threadID.x;
    if (sectionID >= sectionCount)
        return;

    const uint writeOffset = DRAW_INDIRECT_ARGS_COUNT * sectionID + 1;
    drawIndirect[writeOffset] = 0;
}
