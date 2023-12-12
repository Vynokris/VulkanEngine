#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform MatrixBuffer {
    mat4 mvp;
} matrix;

void main()
{
    gl_Position = matrix.mvp * vec4(inPosition, 1.0);
}
