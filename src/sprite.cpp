#include "sprite.hpp"
#include <string>

#include "stb_image/stb_image.h"

int ROGUE_SIZE_X;
int ROGUE_SIZE_Y;
static GLuint SPRITE_ROGUE_MOVE_FORWARD_1;
static GLuint SPRITE_ROGUE_MOVE_FORWARD_2;
static GLuint SPRITE_ROGUE_MOVE_FORWARD_LEFT_1 ;
static GLuint SPRITE_ROGUE_MOVE_FORWARD_LEFT_2;
static GLuint SPRITE_ROGUE_MOVE_LEFT_1;
static GLuint SPRITE_ROGUE_MOVE_LEFT_2;
static GLuint SPRITE_ROGUE_MOVE_BACK_LEFT_1;
static GLuint SPRITE_ROGUE_MOVE_BACK_LEFT_2;
static GLuint SPRITE_ROGUE_MOVE_BACK_1;
static GLuint SPRITE_ROGUE_MOVE_BACK_2;
static GLuint SPRITE_ROGUE_ATTACK;

static GLuint TILE_EARTH_RED;

static GLuint SPRITE_GRASS[5] {};
static GLuint SPRITE_TREE[1] {};
static GLuint SPRITE_BUSH[1] {};


void
new_basic_texture(
	GLuint& texture_id,
	std::string filename
) {
	int channels;
                                auto image_data = stbi_load(
		filename.c_str(), &ROGUE_SIZE_X, &ROGUE_SIZE_Y, &channels, 4
	);

                                glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                                glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		ROGUE_SIZE_X,
		ROGUE_SIZE_Y,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		image_data
	);
}

void load_textures(){
	new_basic_texture(SPRITE_ROGUE_MOVE_FORWARD_1, "./assets/sprites/rogue/forward-1.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_FORWARD_2, "./assets/sprites/rogue/forward-2.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_FORWARD_LEFT_1, "./assets/sprites/rogue/forward-left-1.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_FORWARD_LEFT_2, "./assets/sprites/rogue/forward-left-2.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_BACK_1, "./assets/sprites/rogue/back-1.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_BACK_2, "./assets/sprites/rogue/back-2.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_BACK_LEFT_1, "./assets/sprites/rogue/back-left-1.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_BACK_LEFT_2, "./assets/sprites/rogue/back-left-2.png");
	new_basic_texture(SPRITE_ROGUE_ATTACK, "./assets/sprites/rogue/attack.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_LEFT_1, "./assets/sprites/rogue/left-1.png");
	new_basic_texture(SPRITE_ROGUE_MOVE_LEFT_2, "./assets/sprites/rogue/left-2.png");

	new_basic_texture(TILE_EARTH_RED, "./assets/tiles/earth_block.png");

	new_basic_texture(SPRITE_GRASS[0], "./assets/sprites/grass/1.png");
	new_basic_texture(SPRITE_GRASS[1], "./assets/sprites/grass/2.png");
	new_basic_texture(SPRITE_GRASS[2], "./assets/sprites/grass/3.png");
	new_basic_texture(SPRITE_GRASS[3], "./assets/sprites/grass/4.png");
	new_basic_texture(SPRITE_GRASS[4], "./assets/sprites/grass/7.png");

	new_basic_texture(SPRITE_TREE[0], "./assets/sprites/tree/1.png");
	new_basic_texture(SPRITE_BUSH[0], "./assets/sprites/bush/1.png");
}

void choose_tile_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location
) {
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(texture_flip_location, 0);
	glUniform1i(texture_shader_location, 0);
	glBindTexture(GL_TEXTURE_2D, TILE_EARTH_RED);
}

void choose_grass_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	GLuint count
) {
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(texture_flip_location, count % 2);
	glUniform1i(texture_shader_location, 0);
	glBindTexture(GL_TEXTURE_2D, SPRITE_GRASS[count % 5]);
}

void choose_tree_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	GLuint count
) {
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(texture_flip_location, count % 2);
	glUniform1i(texture_shader_location, 0);
	glBindTexture(GL_TEXTURE_2D, SPRITE_TREE[0]);
}
void choose_bush_texture(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	GLuint count
) {
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(texture_flip_location, count % 2);
	glUniform1i(texture_shader_location, 0);
	glBindTexture(GL_TEXTURE_2D, SPRITE_BUSH[0]);
}
void
choose_rogue_sprite(
	GLuint texture_shader_location,
	GLuint texture_flip_location,
	float direction,
	float path_length,
	bool action
) {

	glActiveTexture(GL_TEXTURE0);

	auto dx = cosf(direction);
	auto dy = sinf(direction);
	auto step = floorf((fmodf(path_length, 0.3f)) / 0.15f);

	glUniform1i(texture_flip_location, 0);
	glUniform1i(texture_shader_location, 0);

	if (action) {
		glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_ATTACK);
		if (dx < 0) {
			glUniform1i(texture_flip_location, 1);
		}
	} else {
		if (dx > abs(dy) * 2.f) {
			glUniform1i(texture_flip_location, 1);
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_LEFT_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_LEFT_2);
			}
		} else if (-dx > abs(dy) * 2.f){
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_LEFT_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_LEFT_2);
			}
		} else if (dy > abs(dx) * 2.f) {
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_FORWARD_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_FORWARD_2);
			}
		} else if (-dy > 2.f * abs(dx)) {
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_BACK_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_BACK_2);
			}
		} else if (2.f * dy > abs(dx) && dx < 0) {
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_FORWARD_LEFT_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_FORWARD_LEFT_2);
			}
		} else if (2.f * dy > abs(dx) && dx > 0.f) {
			glUniform1i(texture_flip_location, 1);
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_FORWARD_LEFT_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_FORWARD_LEFT_2);
			}
		} else if (-2.f * dy > abs(dx) and dx > 0.f) {
			glUniform1i(texture_flip_location, 1);
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_BACK_LEFT_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_BACK_LEFT_2);
			}
		} else if (-2.f * dy > abs(dx) and dx < 0.f) {
			if (step == 0) {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_BACK_LEFT_1);
			} else {
				glBindTexture(GL_TEXTURE_2D, SPRITE_ROGUE_MOVE_BACK_LEFT_2);
			}
		}
	}
}