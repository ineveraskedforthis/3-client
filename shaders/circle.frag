#version 330 core

#define PI 3.14159265358979323846

uniform vec4 albedo;
uniform float direction;
uniform float half_angle;

layout (location = 0) out vec4 out_color;

in vec2 texcoord;

void main()
{
	if (dot(texcoord, texcoord) > 1) {
		discard;
	}

	float local_direction = atan(-texcoord.y, texcoord.x);

	if (
		abs(local_direction - direction) < half_angle
		|| abs(local_direction - direction + 2 * PI) < half_angle
		|| abs(local_direction - direction - 2 * PI) < half_angle
		|| dot(texcoord, texcoord) < 0.1f
	) {
		out_color = albedo * vec4(1.f, 1.f, 1.f, 1.f - dot(texcoord, texcoord));
	} else {
		discard;
	}
}
