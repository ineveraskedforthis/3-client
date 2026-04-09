#define GLEW_STATIC
#include <GL/glew.h>

void load_textures();
void
choose_rat_sprite(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	float direction,
	float path_length,
	bool action
);
void
choose_rogue_sprite(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	float direction,
	float path_length,
	bool action
) ;

void choose_tile_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location
);

void choose_grass_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	GLuint count
) ;

void choose_tree_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	GLuint count
) ;
void choose_bush_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	GLuint count
);