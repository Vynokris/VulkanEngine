#version 450

// Vertex data inputs.
layout(location = 0) in vec3 inPosition;

// Model, view and projection matrices input for the mesh.
layout(set = 0, binding = 0) uniform ModelMatrixBuffer {
    mat4 model;
    mat4 mvp;
} matrices;

// View projection matrix input for the light.
layout(set = 1, binding = 0) uniform LightMatrixBuffer {
    mat4 viewProj;
} light;

void main()
{
    gl_Position = light.viewProj * (matrices.model * vec4(inPosition, 1.0));
}
