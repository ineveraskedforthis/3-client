#version 330 core
layout (location = 0) out vec4 out_color;

uniform vec4 albedo;

void main()
{
	out_color = albedo;
}
