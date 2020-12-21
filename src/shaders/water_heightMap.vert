#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
smooth out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const float pi = 3.14159;
uniform vec3 EyePos;
uniform sampler2D heightMap;
uniform sampler2D interactive;
uniform float amplitude;
uniform bool doInteractive;

float waveHeight(vec2 texCoord) {
    float height = vec3(texture(heightMap, texCoord)).r;
    height = 100 * height * amplitude;
    return height;
}

float waveHeight_interactive(vec2 texCoord) {
    float height = vec3(texture(heightMap, texCoord)).r + (vec3(texture(interactive, texCoord)).r * 5);
    height = 100 * height * amplitude;
    return height;
}


vec3 waveNormal(vec2 texCoord) {
    vec3 n;
    float dx = 0.01;
    float dy = 0.01;

    float heightL = waveHeight(vec2(texCoord[0] + dx, texCoord[1]));
    float heightR = waveHeight(vec2(texCoord[0] - dx, texCoord[1]));
    float heightT = waveHeight(vec2(texCoord[0], texCoord[1] - dy));
    float heightB = waveHeight(vec2(texCoord[0], texCoord[1] + dy));

    vec3 vertical = vec3(2 * dx,  heightL - heightR, 0);
    vec3 horizontal = vec3(0, heightB - heightT, 2 * dy);
    n = normalize(cross(vertical, horizontal));
    if(amplitude == 0){
        n = vec3(0,1,0);
    }
    return n;
}

void main()
{
    vec2 texture_coordinate = aTexCoords;
    vec3 temp_pos = aPos;
    if(doInteractive){
        temp_pos.y = waveHeight_interactive(texture_coordinate);
    }else{
        temp_pos.y = waveHeight(texture_coordinate);
    }

    
    FragPos = vec3(model * vec4(temp_pos, 1.0));
    Normal = waveNormal(texture_coordinate);
    Normal = mat3(transpose(inverse(model))) * Normal;

    gl_Position = projection * view * model * vec4(temp_pos, 1.0);
}