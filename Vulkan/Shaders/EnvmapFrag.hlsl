// Inputs from vertex shader.
struct FSInput
{
    [[vk::location(0)]] float3 vertPos : POSITION;
};

// Fragment color output.
struct FSOutput
{
    float4 color : COLOR;
};

// Cubemap texture input.
[[vk::binding(0, 0)]] TextureCube  cubemapTexture;
[[vk::binding(0, 0)]] SamplerState cubemapSampler;

// Fragment shader main function
FSOutput main(FSInput input)
{
    FSOutput output = (FSOutput)0;

    const float3 cubeDir = normalize(input.vertPos);
    output.color = cubemapTexture.Sample(cubemapSampler, cubeDir);
    
    return output;
}