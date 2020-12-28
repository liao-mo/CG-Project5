#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;

layout (std140, binding = 0) uniform commom_matrices
{
    mat4 projection;
    mat4 view;
};

void main()
{
    TexCoords = aTexCoords;
    gl_Position =projection * view * model * vec4(aPos, 1.0); 
}  