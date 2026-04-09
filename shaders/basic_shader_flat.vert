#version 330 core

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int flip;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

void main()
{
	gl_Position = projection * view * model * vec4(in_position, 1.0);
}
