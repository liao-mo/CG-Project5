#version 430 core
out vec4 FragColor;
uniform sampler2D diffuse;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

void main()
{
    FragColor = texture(diffuse, TexCoords);
}