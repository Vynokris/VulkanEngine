#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(texSampler, fragTexCoord);
    if (fragColor.a == 0.0)
        discard;
}
