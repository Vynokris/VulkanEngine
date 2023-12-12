#version 450

const float PI = 3.14159265359;

// Texture indices definition.
const uint AlbedoTextureIdx   = 0;
const uint EmissiveTextureIdx = 1;
const uint MetallicMapIdx     = 2;
const uint RoughnessMapIdx    = 3;
const uint AOcclusionMapIdx   = 4;
const uint AlphaMapIdx        = 5;
const uint NormalMapIdx       = 6;
const uint DepthMapIdx        = 7;
const uint TextureTypesCount  = 8;

// Light types definition.
const uint DirLightType   = 1;
const uint PointLightType = 2;
const uint SpotLightType  = 3;

// Inputs from vertex shader.
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in mat3 tbnMatrix;

// Push constants input.
layout(push_constant) uniform PushConstants {
    vec3 viewPos;
} pushConstants;

// Fog params input.
layout(set = 1, binding = 0) uniform FogParamsBuffer {
    vec3  color;
    float start;
    float end;
    float invLength;
} fogParams;

// Material data and textures inputs.
layout(set = 2, binding = 0) uniform MaterialBuffer {
    vec3  albedo;
    vec3  emissive;
    float metallic;
    float roughness;
    float alpha;
    float depthMultiplier;
    uint  depthLayerCount;
} materialData;
layout(set = 2, binding = 1) uniform sampler2D materialTextures[TextureTypesCount];

// Light data struct and inputs.
struct Light {
    int   type;
    vec3  albedo;
    vec3  position;
    vec3  direction;
    float brightness, radius, falloff;
    float outerCutoff, innerCutoff;
};
layout(set = 3, binding = 0) uniform LightBuffer { Light data[2]; } lights;

// Fragment color output.
layout(location = 0) out vec4 fragColor;

vec3 ComputeLighting(Light light, vec3 viewDir, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 reflectance);
vec2 ParallaxMapping(vec2 fragTexCoord, vec3 viewDir, float fragDistance);

void main()
{
    // Compute the view direction.
    vec3  fragToView   = pushConstants.viewPos - fragPos;
    vec3  viewDir      = normalize(fragToView);
    float fragDistance = length(fragToView);
    
    // Compute parallax mapping if necessary.
    vec2 texCoord = fragTexCoord;
    if (textureSize(materialTextures[DepthMapIdx], 0).x > 0)
    {
        texCoord = ParallaxMapping(texCoord, normalize(inverse(tbnMatrix) * viewDir), fragDistance);
        if (texCoord.x > 1.0 || texCoord.y > 1.0 || texCoord.x < 0.0 || texCoord.y < 0.0)
            discard;
    }
    
    // Determine fragment transparency from material alpha value and texture.
    fragColor.a = materialData.alpha;
    if (textureSize(materialTextures[AlphaMapIdx], 0).x > 0)
        fragColor.a *= texture(materialTextures[AlphaMapIdx], texCoord).a;

    // Stop computations if the fragment is fully transparent.
    if (fragColor.a <= 0)
        discard; // Might cause visual artifacts.

    // Determine fragment normal from mesh normal and normal map.
    vec3 normal = fragNormal;
    if (textureSize(materialTextures[NormalMapIdx], 0).x > 0) {
        normal = texture(materialTextures[NormalMapIdx], texCoord).rgb * 2.0 - 1.0;
        normal = normalize(tbnMatrix * normal);
    }
    
    // Determine albedo from material albedo value and texture.
    vec3 albedo = materialData.albedo.rgb;
    if (textureSize(materialTextures[AlbedoTextureIdx], 0).x > 0)
        albedo *= texture(materialTextures[AlbedoTextureIdx], texCoord).rgb;
    
    // Determine metallic from material metallic value and map.
    float metallic = materialData.metallic;
    if (textureSize(materialTextures[MetallicMapIdx], 0).x > 0)
        metallic = texture(materialTextures[MetallicMapIdx], texCoord).r;

    // Determine roughness from material roughness value and map.
    float roughness = materialData.roughness;
    if (textureSize(materialTextures[RoughnessMapIdx], 0).x > 0)
        roughness = texture(materialTextures[RoughnessMapIdx], texCoord).r;
    
    // Determine ambient occlusion from ao map.
    float ambientOcclusion = 1;
    if (textureSize(materialTextures[AOcclusionMapIdx], 0).x > 0)
        ambientOcclusion = texture(materialTextures[AOcclusionMapIdx], texCoord).r;

    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 reflectance = mix(vec3(0.04), albedo, metallic);
    
    // Compute lighting.
    vec3 lightSum = vec3(0.0);
    for (int i = 0; i < 5; i++) {
        lightSum += ComputeLighting(lights.data[i], viewDir, normal, albedo, metallic, roughness, reflectance);
    }
    fragColor.rgb = lightSum;
    
    // Add emissive color from material emissive value and texture.
    vec3 emissive = materialData.emissive;
    if (textureSize(materialTextures[EmissiveTextureIdx], 0).x > 0)
        emissive *= texture(materialTextures[EmissiveTextureIdx], texCoord).rgb;
    fragColor.rgb += emissive;
    
    // Compute distance fog.
    fragColor.rgb = mix(fragColor.rgb, fogParams.color, 
                        (clamp(length(fragPos - pushConstants.viewPos), fogParams.start, fogParams.end) 
                        - fogParams.start) * fogParams.invLength);
}

float PointAttenuation(Light light)
{
    float distance = length(light.position - fragPos);
    float s = distance / light.radius;
    if (s >= 1.0)
    return 0.0;
    return pow(1-s*s, 2) / (1 + light.falloff * s);
}

float SpotAttenuation(Light light, vec3 fragToLight)
{
    float cutoff = dot(fragToLight, -light.direction) * 0.5 + 0.5;
    if (cutoff <= 1-light.outerCutoff)
    return 0.0;

    float intensity = ((1-cutoff) - light.outerCutoff) / (light.innerCutoff - light.outerCutoff);
    return PointAttenuation(light) * intensity;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness*roughness;
    float a2 = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float numerator   = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float numerator   = NdotV;
    float denominator = NdotV * (1.0 - k) + k;

    return numerator / denominator;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ComputeLighting(Light light, vec3 viewDir, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 reflectance)
{
    if (light.type != DirLightType && light.type != PointLightType && light.type != SpotLightType)
    return vec3(0.0);

    vec3  fragToLight = light.type == DirLightType ? normalize(-light.direction) : normalize(light.position - fragPos);
    vec3  height      = normalize(viewDir + fragToLight);
    float attenuation = 1.0;
    switch (light.type)
    {
        case PointLightType: attenuation = PointAttenuation(light); break;
        case SpotLightType:  attenuation = SpotAttenuation (light, fragToLight); break;
        default: break;
    }
    vec3 radiance = light.albedo * light.brightness * attenuation;

    float NDF      = DistributionGGX(normal, height, roughness);
    float geoSmith = GeometrySmith  (normal, viewDir, fragToLight, roughness);
    vec3  fresnel  = FresnelSchlick(max(dot(height, viewDir), 0.0), reflectance);

    float normalPolarity = max(dot(normal, fragToLight), 0.0);
    vec3  numerator   = NDF * geoSmith * fresnel;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * normalPolarity + 0.0001; // + 0.0001 to prevent divide by zero
    vec3  specular    = numerator / denominator;
    vec3  diffuse     = (vec3(1.0) - fresnel) * (1.0 - metallic);

    // TODO: The diffuse term is broken.
    return (/*diffuse * */albedo / PI + specular) * radiance * normalPolarity;
}

vec2 ParallaxMapping(vec2 fragTexCoord, vec3 viewDir, float fragDistance)
{
    float maxLayers  = float(materialData.depthLayerCount);
    float numLayers  = max(mix(maxLayers, 0.001, fragDistance * 0.15), 0.001);
    float layerDepth = 1 / numLayers;
    
    vec2 layerOffset   = viewDir.xy  * materialData.depthMultiplier;
    vec2 deltaTexCoord = layerOffset * layerDepth;

    vec2  curTexCoord    = fragTexCoord;
    float curDepthVal    = 1 - texture(materialTextures[DepthMapIdx], curTexCoord).r;
    float curLayerDepth  = 0.0;

    while (curLayerDepth < curDepthVal)
    {
        curTexCoord   -= deltaTexCoord;
        curDepthVal    = 1 - texture(materialTextures[DepthMapIdx], curTexCoord).r;
        curLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = curTexCoord + deltaTexCoord;

    float afterDepth  = curDepthVal - curLayerDepth;
    float beforeDepth = 1 - texture(materialTextures[DepthMapIdx], prevTexCoords).r - curLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    return prevTexCoords * weight + curTexCoord * (1 - weight);
}
