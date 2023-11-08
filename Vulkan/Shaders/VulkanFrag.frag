#version 450

// Texture indices definition.
const uint AlbedoTextureIdx   = 0;
const uint EmissiveTextureIdx = 1;
const uint ShininessMapIdx    = 2;
const uint AlphaMapIdx        = 3;
const uint NormalMapIdx       = 4;
const uint TextureTypesCount  = 5;

// Inputs from vertex shader.
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in mat3 tbnMatrix;

// Push constants input.
layout(push_constant) uniform PushConstants {
    vec3 viewPos;
} pushConstants;

// Material data and textures inputs.
layout(set = 1, binding = 0) uniform MaterialBuffer {
    vec3  albedo;
    vec3  emissive;
    float shininess;
    float alpha;
} materialData;
layout(set = 1, binding = 1) uniform sampler2D materialTextures[TextureTypesCount];

// Light data struct and inputs.
struct Light {
    int   type;
    vec3  albedo;
    vec3  position;
    vec3  direction;
    float brightness;
    float constant, linear, quadratic;
    float outerCutoff, innerCutoff;
};
layout(set = 2, binding = 0) uniform LightBuffer { Light data[2]; } lights;

// Fragment color output.
layout(location = 0) out vec4 fragColor;

void ComputeLightingParams(vec3 normal, vec3 viewDir, vec3 fragToLight, float shininess, inout float diff, inout float spec)
{
    vec3 reflectDir = reflect(fragToLight, normal);
    diff = max(dot(normal, fragToLight), 0.0);
    spec = pow(max(dot(-viewDir, reflectDir), 0.0), shininess);
}

vec3 ComputeDirLight(Light light, vec3 viewDir, vec3 normal, vec3 albedo, float shininess)
{
    // Compute lighting parameters.
    vec3  combinedAlbedo = light.albedo * albedo;
    vec3  fragToLight    = normalize(-light.direction);
    float diff           = 0.0;
    float spec           = 0.0;
    ComputeLightingParams(normal, viewDir, fragToLight, shininess, diff, spec);

    // Combine albedo with brightness, diffuse and specular.
    return combinedAlbedo * light.brightness * (diff + spec);
}

vec3 ComputePointLight(Light light, vec3 viewDir, vec3 normal, vec3 albedo, float shininess)
{
    // Compute lighting parameters.
    vec3  combinedAlbedo = light.albedo * albedo;
    vec3  fragToLight    = normalize(light.position - fragPos);
    float diff           = 0.0;
    float spec           = 0.0;
    ComputeLightingParams(normal, viewDir, fragToLight, shininess, diff, spec);

    // Compute attenuation.
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // Combine albedo with brightness, attenuation, diffuse and specular.
    return combinedAlbedo * light.brightness * attenuation * (diff + spec);
}

vec3 ComputeSpotLight(Light light, vec3 viewDir, vec3 normal, vec3 albedo, float shininess)
{
    // Stop if the fragment isn't in the light cone.
    vec3  fragToLight = normalize(light.position - fragPos);
    float cutoff      = dot(fragToLight, -light.direction) * 0.5 + 0.5;
    if (cutoff <= 1-light.outerCutoff)
        return vec3(0.0);

    // Compute lighting parameters.
    vec3  combinedAlbedo = light.albedo * albedo;
    float diff           = 0.0;
    float spec           = 0.0;
    ComputeLightingParams(normal, viewDir, fragToLight, shininess, diff, spec);

    // Compute attenuation.
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    float intensity   = ((1-cutoff) - light.outerCutoff) / (light.innerCutoff - light.outerCutoff);

    // Combine albedo with brightness, attenuation, intensity, diffuse and specular.
    return combinedAlbedo * light.brightness * attenuation * intensity * (diff + spec);
}

void main()
{
    // Determine fragment transparency from material alpha value and texture.
    fragColor.a = materialData.alpha;
    if (textureSize(materialTextures[AlphaMapIdx], 0).x > 0)
        fragColor.a *= texture(materialTextures[AlphaMapIdx], fragTexCoord).a;

    // Stop computations if the fragment is fully transparent.
    if (fragColor.a <= 0)
        return; // Might cause visual artifacts.
    
    // Compute the view direction.
    vec3 viewDir = normalize(pushConstants.viewPos - fragPos);

    // Get normal from normal map.
    vec3 normal = fragNormal;
    if (textureSize(materialTextures[NormalMapIdx], 0).x > 0) {
        normal = normalize(tbnMatrix * (texture(materialTextures[NormalMapIdx], fragTexCoord).rgb * 2.0 - 1.0));
    }
    
    // Determine fragment color from material albedo value and texture.
    vec3 albedo = materialData.albedo.rgb;
    if (textureSize(materialTextures[AlbedoTextureIdx], 0).x > 0)
        albedo *= texture(materialTextures[AlbedoTextureIdx], fragTexCoord).rgb;
    
    // Get shininess from shininess map.
    float shininess = materialData.shininess;
    if (textureSize(materialTextures[ShininessMapIdx], 0).x > 0)
        shininess = dot(texture(materialTextures[ShininessMapIdx], fragTexCoord), vec4(1)) * 0.25;
    
    // Compute lighting.
    fragColor.rgb = vec3(0.0);
    for (int i = 0; i < 5; i++)
    {
        switch (lights.data[i].type)
        {
            case 1: fragColor.rgb += ComputeDirLight  (lights.data[i], viewDir, normal, albedo, shininess); break;
            case 2: fragColor.rgb += ComputePointLight(lights.data[i], viewDir, normal, albedo, shininess); break;
            case 3: fragColor.rgb += ComputeSpotLight (lights.data[i], viewDir, normal, albedo, shininess); break;
            default: break;
        }
    }
    
    // Add emissive color from material emissive value and texture.
    vec3 emissive = materialData.emissive;
    if (textureSize(materialTextures[EmissiveTextureIdx], 0).x > 0)
        emissive *= texture(materialTextures[EmissiveTextureIdx], fragTexCoord).rgb;
    fragColor.rgb += emissive;
}
