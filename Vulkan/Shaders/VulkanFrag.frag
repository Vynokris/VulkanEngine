#version 450

layout(location = 0) in  vec3 VertColor;
layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(VertColor, 1.0);
}
