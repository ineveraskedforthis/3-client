#version 330 core

uniform sampler2D albedo_texture;

layout (location = 0) out vec4 out_color;

in vec2 texcoord;

void main()
{
	vec4 texture_value = texture(albedo_texture, texcoord);
	if (texture_value.a <= 0.2) {
		discard;
	}
	out_color = texture_value;
}
