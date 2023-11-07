#version 450

// Vertex data inputs.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// Model, view and projection matrices input.
layout(set = 0, binding = 0) uniform MatricesBuffer {
    mat4 model;
    mat4 mvp;
} matrices;

// Outputs to fragment shader.
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out mat3 tbnMatrix;

void main() 
{
    gl_Position    = matrices.mvp * vec4(inPosition, 1.0);
    fragPos        = (matrices.model * vec4(inPosition, 1.0)).xyz;
    fragTexCoord   = inTexCoord;
    fragNormal     = normalize((matrices.model * vec4(inNormal,    0.0)).xyz);
    vec3 tangent   = normalize((matrices.model * vec4(inTangent,   0.0)).xyz);
    vec3 bitangent = normalize((matrices.model * vec4(inBitangent, 0.0)).xyz);
    tbnMatrix      = mat3(tangent, bitangent, fragNormal);
}
