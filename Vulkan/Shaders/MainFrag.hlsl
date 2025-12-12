const float PI = 3.14159265359;

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

// Light types definition.
#define DirLightType   1
#define PointLightType 2
#define SpotLightType  3

// Inputs from vertex shader.
struct FSInput
{
    [[vk::location(0)]] float3   fragPos   : POSITION;
    [[vk::location(1)]] float2   texCoord  : TEXCOORD;
    [[vk::location(2)]] float3   normal    : NORMAL;
    [[vk::location(3)]] float3x3 tbnMatrix : NORMAL1; // This variable uses 3 locations in total.
};

// Fragment color output.
struct FSOutput
{
    float4 color : COLOR;
};

// Push constants input.
struct PushConstants
{
    float3 viewPos;
};
[[vk::push_constant]] PushConstants pushConstants;

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

float3  ComputeLighting(Light light, float3 fragPos, float3 viewDir, float3 normal, float3 albedo, float metallic, float roughness, float3 reflectance);
float2  ParallaxMapping(float2 fragTexCoord, float3 viewDir, float fragDistance);

FSOutput main(FSInput input)
{
    FSOutput output = (FSOutput)0;
    
    // Compute the view direction.
    const float3 fragToView   = pushConstants.viewPos - input.fragPos;
    const float3 viewDir      = normalize(fragToView);
    const float  fragDistance = length(fragToView);
    
    // Compute parallax mapping if necessary.
    float2 texCoord = input.texCoord;
    uint width, height, levelCount;
    materialTextures[DepthMapIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0)
    {
        texCoord = ParallaxMapping(texCoord, normalize(/*inverse*/transpose(input.tbnMatrix) * viewDir), fragDistance);
    }
    else
    {
        const float uvScale = materialData.depthMultiplier * 10.0;
        texCoord *= uvScale;
    }
    
    // Determine fragment transparency from material alpha value and texture.
    output.color.a = materialData.alpha;
    materialTextures[AlphaMapIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0)
        output.color.a *= materialTextures[AlphaMapIdx].Sample(materialSamplers[AlphaMapIdx], texCoord).a;
    
    // Determine albedo from material albedo value and texture.
    float3 albedo = materialData.albedo.rgb;
    materialTextures[AlbedoTextureIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0) {
        float4 texSample = materialTextures[AlbedoTextureIdx].Sample(materialSamplers[AlbedoTextureIdx], texCoord);
        albedo *= texSample.rgb;
        output.color.a *= texSample.a;
    }

    // Stop computations if the fragment is fully transparent.
    if (output.color.a <= 0)
        discard; // Might cause visual artifacts.
    
    // Determine metallic from material metallic value and map.
    float metallic = materialData.metallic;
    materialTextures[MetallicMapIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0)
        metallic *= materialTextures[MetallicMapIdx].Sample(materialSamplers[MetallicMapIdx], texCoord).r;

    // Determine roughness from material roughness value and map.
    float roughness = materialData.roughness;
    materialTextures[RoughnessMapIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0)
        roughness *= materialTextures[RoughnessMapIdx].Sample(materialSamplers[RoughnessMapIdx], texCoord).r;
    
    // Determine ambient occlusion from ao map.
    float ambientOcclusion = 1;
    materialTextures[AOcclusionMapIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0)
        ambientOcclusion = materialTextures[AOcclusionMapIdx].Sample(materialSamplers[AOcclusionMapIdx], texCoord).r;

    // Determine fragment normal from mesh normal and normal map.
    float3 normal = input.normal.xyz;
    materialTextures[NormalMapIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0) {
        normal = materialTextures[NormalMapIdx].Sample(materialSamplers[NormalMapIdx], texCoord).rgb * 2.0 - 1.0;
        normal = normalize(input.tbnMatrix * normal);
    }

    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    const float3 reflectance = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    
    // Compute lighting and shadows.
    float3 lightSum = float3(0,0,0);
    for (int i = 0; i < 5; i++) {
        lightSum += ComputeLighting(lights.data[i], input.fragPos, viewDir, normal, albedo, metallic, roughness, reflectance);
    }
    output.color.rgb = lightSum * ambientOcclusion;
    
    // Add emissive color from material emissive value and texture.
    float3 emissive = materialData.emissive;
    materialTextures[EmissiveTextureIdx].GetDimensions(0, width, height, levelCount);
    if (width > 0)
        emissive *= materialTextures[EmissiveTextureIdx].Sample(materialSamplers[EmissiveTextureIdx], texCoord).rgb;
    output.color.rgb += emissive;
    
    // Compute distance fog.
    output.color.rgb = lerp(output.color.rgb, fogParams.color, 
                        (clamp(length(input.fragPos - pushConstants.viewPos), fogParams.start, fogParams.end) 
                        - fogParams.start) * fogParams.invLength);
    
    // Apply gamma correction.
    output.color.rgb = pow(output.color.rgb, 1.0/1.6);
    return output;
}

float PointAttenuation(Light light, float3 fragPos)
{
    const float distance = length(light.position - fragPos);
    const float s = distance / light.radius;
    if (s >= 1.0)
        return 0.0;
    return pow(1-s*s, 2) / (1 + light.falloff * s);
}

float SpotAttenuation(Light light, float3 fragPos, float3 fragToLight)
{
    const float cutoff = dot(fragToLight, -light.direction) * 0.5 + 0.5;
    if (cutoff <= 1-light.outerCutoff)
        return 0.0;

    const float intensity = ((1-cutoff) - light.outerCutoff) / (light.innerCutoff - light.outerCutoff);
    return PointAttenuation(light, fragPos) * intensity;
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    const float a  = roughness*roughness;
    const float a2 = a*a;
    const float NdotH  = max(dot(N, H), 0.0);
    const float NdotH2 = NdotH*NdotH;

    const float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    const float r = (roughness + 1.0);
    const float k = (r*r) / 8.0;

    const float numerator   = NdotV;
    const float denominator = NdotV * (1.0 - k) + k;

    return numerator / denominator;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    const float NdotV = max(dot(N, V), 0.0);
    const float NdotL = max(dot(N, L), 0.0);
    const float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    const float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 ComputeLighting(Light light, float3 fragPos, float3 viewDir, float3 normal, float3 albedo, float metallic, float roughness, float3 reflectance)
{
    if (light.type != DirLightType && light.type != PointLightType && light.type != SpotLightType)
    return float3(0,0,0);

    const float3 fragToLight = light.type == DirLightType ? normalize(-light.direction) : normalize(light.position - fragPos);
    const float3 height      = normalize(viewDir + fragToLight);
    float  attenuation = 1.0;
    switch (light.type)
    {
        case PointLightType: attenuation = PointAttenuation(light, fragPos); break;
        case SpotLightType:  attenuation = SpotAttenuation (light, fragPos, fragToLight); break;
        default: break;
    }
    const float3 radiance = light.albedo.rgb * light.albedo.a * attenuation;

    const float  NDF      = DistributionGGX(normal, height, roughness);
    const float  geoSmith = GeometrySmith  (normal, viewDir, fragToLight, roughness);
    const float3 fresnel  = FresnelSchlick(max(dot(height, viewDir), 0.0), reflectance);

    const float  normalPolarity = max(dot(normal, fragToLight), 0.0);
    const float3 numerator      = NDF * geoSmith * fresnel;
    const float  denominator    = 4.0 * max(dot(normal, viewDir), 0.0) * normalPolarity + 0.0001; // + 0.0001 to prevent divide by zero
    const float3 specular       = numerator / denominator;
    const float3 diffuse        = (float3(1,1,1) - fresnel) * (1.0 - metallic);
    
    // TODO: The diffuse term is broken.
    return (diffuse * albedo / PI + specular) * radiance * normalPolarity;
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
    float  curLayerDepth = 0.0;

    while (curLayerDepth < curDepthVal)
    {
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
