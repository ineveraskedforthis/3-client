#define GLEW_STATIC
#include <GL/glew.h>

void load_rogue_textures();

void
choose_rogue_sprite(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	float direction,
	float path_length,
	bool action
) ;