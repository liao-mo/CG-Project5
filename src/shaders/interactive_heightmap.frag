#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

const float PI = 3.1415926;

uniform int mode;
uniform sampler2D u_water;
uniform vec2 u_center;
uniform float u_radius;
uniform float u_strength;


void main()
{
	if(mode == 0){ // initialization
		FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else if(mode == 1){ // drop
		vec4 info = texture2D(u_water, TexCoords);

		float drop = max(0.0, 1.0 - length(u_center - TexCoords) / u_radius);
		drop = 0.5 - cos(drop * PI) * 0.5;
		info.r += drop * u_strength;
		FragColor = info;
	}
	else if(mode == 2){ //update
		vec4 info = texture2D(u_water, TexCoords);

		vec2 dx = vec2(0.01, 0.0);
		vec2 dy = vec2(0.0, 0.01);
		float average = (
		texture2D(u_water, TexCoords - dx).r + 
		texture2D(u_water, TexCoords - dy).r + 
		texture2D(u_water, TexCoords + dx).r + 
		texture2D(u_water, TexCoords + dy).r) * 0.25;

		info.g += (average - info.r) * 1.0;
		info.g *= 0.97;
		info.r += info.g;
		info.r *= 0.95;

		FragColor = info;
	}
}   