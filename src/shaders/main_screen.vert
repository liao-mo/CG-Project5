#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform bool shake;
uniform float time;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 

    if (shake)
    {
        float strength = 0.02;
        gl_Position.x += cos(time * 20) * strength;        
        gl_Position.y += cos(time * 30) * strength;        
    }
}  