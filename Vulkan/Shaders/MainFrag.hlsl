#define PI  3.14159265359f

// Texture indices definition.
#define AlbedoTextureIdx   0
#define EmissiveTextureIdx 1
#define MetallicMapIdx     2
#define RoughnessMapIdx    3
#define AOcclusionMapIdx   4
#define AlphaMapIdx        5
#define NormalMapIdx       6
#define DepthMapIdx        7
#define TextureTypesCount  8

// Inserts an if check that passes if the given texture is set.
#define if_HasTexture(texIdx) \
    materialTextures[texIdx].GetDimensions(0, width, height, levelCount); \
    if (width > 0)

// Samples the given texture at the given tex coords.
#define SampleTexture(texIdx, texCoord) \
    materialTextures[texIdx].Sample(materialSamplers[texIdx], texCoord)

// Light types definition.
#define DirLightType   1
#define PointLightType 2
#define SpotLightType  3

// Inputs from vertex shader.
struct FSInput
{
    [[vk::location(0)]] float3   fragPos   : POSITION;
    [[vk::location(1)]] float2   texCoord  : TEXCOORD;
    [[vk::location(2)]] float3x3 tbnMatrix : NORMAL; // This variable uses 3 locations in total.
};

// Fragment color output.
struct FSOutput
{
    float4 color : COLOR;
};

// Push constants input.
[[vk::push_constant]] cbuffer pushConstants
{
    row_major float4x4 viewProj;
    float3 viewPos;
};

// Fog params input.
struct FogParams
{
    float3 color;
    float  start;
    float  end;
    float  invLength;
};
[[vk::binding(0, 1)]] ConstantBuffer<FogParams> fogParams;

// Material data and textures inputs.
struct MaterialData
{
    float3 albedo;
    float3 emissive;
    float  metallic;
    float  roughness;
    float  alpha;
    float  depthMultiplier;
    uint   depthLayerCount;
};
[[vk::binding(0, 2)]] ConstantBuffer<MaterialData> materialData;
[[vk::binding(1, 2)]] Texture2D    materialTextures[TextureTypesCount];
[[vk::binding(1, 2)]] SamplerState materialSamplers[TextureTypesCount];

// Light data struct and inputs.
struct Light
{
    float4 albedo;
    float3 position;
    float3 direction;
    float  radius, falloff;
    float  outerCutoff, innerCutoff;
    int    type;
};
struct LightBuffer
{
    Light data[2];
};
[[vk::binding(0, 3)]] ConstantBuffer<LightBuffer> lights;

// Cubemap texture input.
[[vk::binding(0, 4)]] TextureCube  cubemapTexture;
[[vk::binding(0, 4)]] SamplerState cubemapSampler;

float3 ComputeLighting(Light light, float3 fragPos, float3 viewDir, float3 normal, float3 albedo, float metallic, float roughness, float3 reflectance);
float3 ComputeCubemap (float3 fragPos, float3 viewDir, float3 normal, float3 albedo, float metallic, float roughness, float3 reflectance);
float2 ParallaxMapping(float2 fragTexCoord, float3 viewDir, float fragDistance);

// Fragment shader main function
FSOutput main(FSInput input)
{
    // Define output, and variables used to fetch texture dimensions.
    FSOutput output = (FSOutput)0;
    uint width, height, levelCount;
    
    // Compute the view direction.
    const float3 fragToView   = viewPos - input.fragPos;
    const float3 viewDir      = normalize(fragToView);
    const float  fragDistance = length(fragToView);
    
    // Compute parallax mapping if necessary.
    float2 texCoord = input.texCoord;
    if_HasTexture(DepthMapIdx)
        texCoord = ParallaxMapping(texCoord, normalize(transpose(input.tbnMatrix) * viewDir), fragDistance);
    
    // Determine fragment transparency from material alpha value and texture.
    output.color.a = materialData.alpha;
    if_HasTexture(AlphaMapIdx)
        output.color.a *= SampleTexture(AlphaMapIdx, texCoord).a;
    
    // Determine albedo from material albedo value and texture.
    float3 albedo = materialData.albedo.rgb;
    if_HasTexture(AlbedoTextureIdx) {
        float4 texSample = SampleTexture(AlbedoTextureIdx, texCoord);
        albedo *= texSample.rgb;
        output.color.a *= texSample.a;
    }

    // Stop computations if the fragment is fully transparent.
    if (output.color.a <= 0)
        discard;
    
    // Determine metallic from material metallic value and map.
    float metallic = materialData.metallic;
    if_HasTexture(MetallicMapIdx)
        metallic *= SampleTexture(MetallicMapIdx, texCoord).r;

    // Determine roughness from material roughness value and map.
    float roughness = materialData.roughness;
    if_HasTexture(RoughnessMapIdx)
        roughness *= SampleTexture(RoughnessMapIdx, texCoord).r;
    
    // Determine ambient occlusion from ao map.
    float ambientOcclusion = 1;
    if_HasTexture(AOcclusionMapIdx)
        ambientOcclusion = SampleTexture(AOcclusionMapIdx, texCoord).r;

    // Determine fragment normal from mesh normal and normal map.
    float3 normal = input.tbnMatrix[2];
    if_HasTexture(NormalMapIdx) {
        normal = SampleTexture(NormalMapIdx, texCoord).rgb * 2 - 1;
        normal = normalize(input.tbnMatrix * normal);
    }

    // Calculate reflectance at normal incidence.
    // For high metalness, use albedo as F0 (metallic workflow)
    // For low metalness, use F0 of 0.04
    const float3 reflectance = lerp(0.04, albedo, metallic);
    
    // Compute sum of lighting contributions for all lights and cubemap.
    float3 lightSum = ComputeCubemap(input.fragPos, viewDir, normal, albedo, metallic, roughness, reflectance);
    //for (int i = 0; i < 5; i++) {
    //    lightSum += ComputeLighting(lights.data[i], input.fragPos, viewDir, normal, albedo, metallic, roughness, reflectance);
    //}
    output.color.rgb = lightSum * ambientOcclusion;
    
    // Add emissive color from material emissive value and texture.
    float3 emissive = materialData.emissive;
    if_HasTexture(EmissiveTextureIdx)
        emissive *= SampleTexture(EmissiveTextureIdx, texCoord).rgb;
    output.color.rgb += emissive;
    
    // Compute distance fog.
    output.color.rgb = lerp(output.color.rgb, fogParams.color, 
                        (clamp(length(input.fragPos - viewPos), fogParams.start, fogParams.end) 
                        - fogParams.start) * fogParams.invLength);
    
    // Apply gamma correction.
    output.color.rgb = pow(output.color.rgb, 1/1.6);

    return output;
}

float PointAttenuation(Light light, float3 fragPos)
{
    const float distance = length(light.position - fragPos);
    const float s = distance / light.radius;
    if (s >= 1)
        return 0;
    return pow(1-s*s, 2) / (1 + light.falloff * s);
}

float SpotAttenuation(Light light, float3 fragPos, float3 fragToLight)
{
    const float cutoff = dot(fragToLight, -light.direction) * 0.5 + 0.5;
    if (cutoff <= 1-light.outerCutoff)
        return 0;

    const float intensity = ((1-cutoff) - light.outerCutoff) / (light.innerCutoff - light.outerCutoff);
    return PointAttenuation(light, fragPos) * intensity;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    const float a  = roughness*roughness;
    const float a2 = a*a;
    const float NdotH  = max(dot(N, H), 0);
    const float NdotH2 = NdotH*NdotH;

    const float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1) + 1);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    const float r = (roughness + 1);
    const float k = (r*r) / 8;

    const float numerator   = NdotV;
    const float denominator = NdotV * (1 - k) + k;

    return numerator / denominator;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    const float NdotV = max(dot(N, V), 0);
    const float NdotL = max(dot(N, L), 0);
    const float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    const float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1 - F0) * pow(clamp(1 - cosTheta, 0, 1), 5);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1 - roughness, F0) - F0) * pow(clamp(1 - cosTheta, 0, 1), 5);
}

float3 ComputeLighting(Light light, float3 fragPos, float3 viewDir, float3 normal, float3 albedo, float metallic, float roughness, float3 reflectance)
{
    if (all(light.albedo == 0) || (light.type != DirLightType && light.type != PointLightType && light.type != SpotLightType))
        return 0;

    const float3 fragToLight = light.type == DirLightType ? normalize(-light.direction) : normalize(light.position - fragPos);
    const float3 height      = normalize(viewDir + fragToLight);
    float  attenuation = 1;
    switch (light.type)
    {
        case PointLightType: attenuation = PointAttenuation(light, fragPos); break;
        case SpotLightType:  attenuation = SpotAttenuation (light, fragPos, fragToLight); break;
        default: break;
    }
    const float3 radiance = light.albedo.rgb * light.albedo.a * attenuation;

    const float  NDF      = DistributionGGX(normal, height, roughness);
    const float  geoSmith = GeometrySmith  (normal, viewDir, fragToLight, roughness);
    const float3 fresnel  = FresnelSchlick(max(dot(height, viewDir), 0), reflectance);

    const float  normalPolarity = max(dot(normal, fragToLight), 0);
    const float3 numerator      = NDF * geoSmith * fresnel;
    const float  denominator    = 4 * max(dot(normal, viewDir), 0) * normalPolarity + 0001; // + 0001 to prevent divide by zero
    const float3 specular       = numerator / denominator;
    const float3 diffuse        = (1 - fresnel) * (1 - metallic);
    
    return (diffuse * albedo / PI + specular) * radiance * normalPolarity;
}

float3 ComputeCubemap(float3 fragPos, float3 viewDir, float3 normal, float3 albedo, float metallic, float roughness, float3 reflectance)
{
    uint texWidth, texHeight, texLevelCount;
    cubemapTexture.GetDimensions(0, texWidth, texHeight, texLevelCount);
    if (texWidth == 0)
        return 0;

    const float3 fragToLight = reflect(-viewDir, normal);
    const float3 height      = normalize(viewDir + fragToLight);
    const float  roughExp    = 1 - pow(1 - roughness, 4);
    const float  NDF         = DistributionGGX(normal, height, roughExp);
    const float  geoSmith    = GeometrySmith  (normal, viewDir, fragToLight, roughExp);
    const float3 fresnel     = FresnelSchlickRoughness(max(dot(height, viewDir), 0), reflectance, roughExp);

    const float  normalPolarity = max(dot(normal, fragToLight), 0);
    const float3 numerator      = NDF * geoSmith * fresnel;
    const float  denominator    = 4 * max(dot(normal, viewDir), 0) * normalPolarity + 0001; // + 0001 to prevent divide by zero
    const float3 specular       = numerator / denominator;
    const float3 diffuse        = (1 - fresnel) * (1 - metallic);
    
    const uint   maxLevel    = texLevelCount - 1;
    const float  roughLod    = maxLevel * roughness;
    const float3 skyDiffuse  = cubemapTexture.SampleLevel(cubemapSampler, normal, maxLevel).rgb;
    const float3 skySpecular = cubemapTexture.SampleLevel(cubemapSampler, fragToLight, roughLod).rgb;
    
    return skyDiffuse * diffuse * albedo / PI + skySpecular * specular;
}

float2 ParallaxMapping(float2 fragTexCoord, float3 viewDir, float fragDistance)
{
    const float maxLayers  = float(materialData.depthLayerCount);
    const float numLayers  = max(lerp(maxLayers, 1, log(fragDistance+1) * 0.5), 1);
    const float layerDepth = 1 / numLayers;
    
    const float2 layerOffset   = viewDir.xy  * materialData.depthMultiplier;
    const float2 deltaTexCoord = layerOffset * layerDepth;

    float2 curTexCoord   = fragTexCoord;
    float  curDepthVal   = 1 - materialTextures[DepthMapIdx].Sample(materialSamplers[DepthMapIdx], curTexCoord).r;
    float  curLayerDepth = 0;

    for (uint i = 0; i < 32; i++)
    {
        if (curLayerDepth >= curDepthVal)
            break;
        
        curTexCoord   -= deltaTexCoord;
        curDepthVal    = 1 - materialTextures[DepthMapIdx].Sample(materialSamplers[DepthMapIdx], curTexCoord).r;
        curLayerDepth += layerDepth;
    }

    const float2 prevTexCoords = curTexCoord + deltaTexCoord;

    const float afterDepth  = curDepthVal - curLayerDepth;
    const float beforeDepth = 1 - materialTextures[DepthMapIdx].Sample(materialSamplers[DepthMapIdx], prevTexCoords).r - curLayerDepth + layerDepth;

    const float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + curTexCoord * (1 - weight);
}
