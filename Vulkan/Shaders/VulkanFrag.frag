#version 450

const uint AlbedoTextureIdx   = 0;
const uint EmissiveTextureIdx = 1;
const uint ShininessMapIdx    = 2;
const uint AlphaMapIdx        = 3;
const uint NormalMapIdx       = 4;
const uint TextureTypesCount  = 5;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;

layout(set = 1, binding = 0) uniform MaterialBuffer {
    vec3 albedo;
    vec3 emissive;
    float shininess;
    float alpha;
} materialData;
layout(set = 1, binding = 1) uniform sampler2D materialTextures[TextureTypesCount];

layout(location = 0) out vec4 fragColor;

void main()
{
    // Determine fragment transparency from material alpha value and texture.
    fragColor.a = materialData.alpha;
    if (textureSize(materialTextures[AlphaMapIdx], 0).x > 0)
        fragColor.a *= texture(materialTextures[AlphaMapIdx], fragTexCoord).a;

    // Stop computations if the fragment is fully transparent.
    if (fragColor.a <= 0)
        return; // Might cause visual artifacts.
    
    // Determine fragment color from material albedo value and texture.
    fragColor.rgb = materialData.albedo.rgb;
    if (textureSize(materialTextures[AlbedoTextureIdx], 0).x > 0)
        fragColor.rgb *= texture(materialTextures[AlbedoTextureIdx], fragTexCoord).rgb;
    
    // Add emissive color from material emissive value and texture.
    vec3 emissive = materialData.emissive;
    if (textureSize(materialTextures[EmissiveTextureIdx], 0).x > 0)
        emissive *= texture(materialTextures[EmissiveTextureIdx], fragTexCoord).rgb;
    fragColor.rgb += emissive;
}
