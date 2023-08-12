#version 450

layout(set = 0, binding = 0) uniform MatricesBuffer {
    mat4 model;
    mat4 view;
    mat4 proj;
} matrices;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;

void main() 
{
    gl_Position  = matrices.proj * matrices.view * matrices.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    fragNormal   = inNormal;
}
