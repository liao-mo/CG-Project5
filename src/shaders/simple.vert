#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texture_coordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


out V_OUT
{
   vec3 position;
   vec3 normal;
   vec2 texture_coordinate;
} v_out;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0f);

    v_out.position = vec3(model * vec4(position, 1.0f));
    v_out.normal = mat3(transpose(inverse(model))) * normal;
    v_out.texture_coordinate = vec2(texture_coordinate.x, 1.0f - texture_coordinate.y);
}