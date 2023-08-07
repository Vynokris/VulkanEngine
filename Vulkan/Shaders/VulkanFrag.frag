#version 450

const uint AlbedoTextureIdx   = 0;
const uint EmissiveTextureIdx = 1;
const uint ShininessMapIdx    = 2;
const uint AlphaMapIdx        = 3;
const uint NormalMapIdx       = 4;
const uint TextureTypesCount  = 5;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;

layout(binding = 1) uniform sampler2D materialTextures[TextureTypesCount];

layout(location = 0) out vec4 fragColor;

void main()
{
    if (textureSize(materialTextures[NormalMapIdx], 0).x > 0)
        fragColor = texture(materialTextures[NormalMapIdx], fragTexCoord);
    else
        fragColor = texture(materialTextures[AlbedoTextureIdx], fragTexCoord);
}
