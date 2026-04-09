#include "data_ids.hpp"
#include <optional>
#define GLM_FORCE_SWIZZLE
#define GLEW_STATIC

#include <array>
#include <assert.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

#define DEFAULT_BUFLEN 512

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>

#include "stb_image/stb_image.h"



#include "unordered_dense.h"

// Include all GLM core / GLSL features
#include <glm/glm.hpp> // vec2, vec3, mat4, radians
// Include all GLM extensions
#include <glm/ext.hpp> // perspective, translate, rotate
#include "glm/ext/matrix_transform.hpp"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "implot/implot.h"
#include "implot/implot_internal.h"

#include "data.hpp"

#include "frustum.hpp"

#include "sprite.hpp"

constexpr inline float TILE_SIZE = 64.f;

constexpr inline float GRASS_TILE_SIZE = 16.f;

void APIENTRY glDebugOutput(
	GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char *message,
	const void *userParam
) {
	// ignore non-significant error/warning codes
	if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " <<  message << std::endl;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
		case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
		case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
		case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
		case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
		case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}

struct buffer_pair {
	GLuint vao;
	GLuint vbo;
};

std::string_view opengl_get_error_name(GLenum t) {
	switch(t) {
		case GL_INVALID_ENUM:
			return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:
			return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:
			return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:
			return "GL_OUT_OF_MEMORY";
		case GL_STACK_UNDERFLOW:
			return "GL_STACK_UNDERFLOW";
		case GL_STACK_OVERFLOW:
			return "GL_STACK_OVERFLOW";
		case GL_NO_ERROR:
			return "GL_NO_ERROR";
		default:
			return "Unknown";
	}
}
std::string to_string(std::string_view str) {
	return std::string(str.begin(), str.end());
}
void opengl_error_print(std::string message) {
	std::string full_message = message;
	full_message += "\n";
	full_message += opengl_get_error_name(glGetError());
	printf("%s\n", ("OpenGL error:" + full_message).c_str());
}
void assert_no_errors() {
	auto error = glGetError();
	if (error != GL_NO_ERROR) {
		auto message = opengl_get_error_name(error);
		printf("%s\n", (to_string(message)).c_str());
		assert(false);
	}
}

const std::string read_shader(const std::string path) {
	std::string shader_source;
	std::ifstream shader_file;

	shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		shader_file.open(path);
		std::stringstream shader_source_stream;
		shader_source_stream << shader_file.rdbuf();
		shader_file.close();
		shader_source = shader_source_stream.str();
	} catch (std::ifstream::failure& e) {
		throw std::runtime_error(e);
	}

	return shader_source;
}

GLuint create_shader(GLenum type, const char *source) {
	GLuint result = glCreateShader(type);
	glShaderSource(result, 1, &source, nullptr);
	glCompileShader(result);
	GLint status;
	glGetShaderiv(result, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		GLint info_log_length;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log(info_log_length, '\0');
		glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
		throw std::runtime_error("Shader compilation failed: " + info_log);
	}
	return result;
}

template <typename ... Shaders>
GLuint create_program(Shaders ... shaders)
{
	GLuint result = glCreateProgram();
	(glAttachShader(result, shaders), ...);
	glLinkProgram(result);

	GLint status;
	glGetProgramiv(result, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		GLint info_log_length;
		glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
		std::string info_log(info_log_length, '\0');
		glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
		throw std::runtime_error("Program linkage failed: " + info_log);
	}

	return result;
}


void glew_fail(std::string_view message, GLenum error) {
	throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

static int current_move_x = 0;
static int current_move_y = 0;

static int prev_move_x = 0;
static int prev_move_y = 0;

static bool running = false;
static bool prev_running = false;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_UP) {
		if (action == GLFW_PRESS) {
			current_move_y += -1;
		} else if (action == GLFW_RELEASE) {
			current_move_y -= -1;
		}
	}

	if (key == GLFW_KEY_DOWN) {
		if (action == GLFW_PRESS) {
			current_move_y -= -1;
		} else if (action == GLFW_RELEASE) {
			current_move_y += -1;
		}
	}

	if (key == GLFW_KEY_LEFT) {
		if (action == GLFW_PRESS) {
			current_move_x -= 1;
		} else if (action == GLFW_RELEASE) {
			current_move_x += 1;
		}
	}

	if (key == GLFW_KEY_RIGHT) {
		if (action == GLFW_PRESS) {
			current_move_x += 1;
		} else if (action == GLFW_RELEASE) {
			current_move_x -= 1;
		}
	}

	if (key == GLFW_KEY_LEFT_SHIFT) {
		if (action == GLFW_PRESS) {
			running = true;
		} else if (action == GLFW_RELEASE) {
			running = false;
		}
	}
}

struct vertex {
	glm::vec3 position;
	glm::vec3 normal_direction;
	glm::vec2 texture_coordinate;
};

static std::vector<vertex> world_object_vertical_mesh = {
	{{-0.5f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 1.f}},
	{{0.5f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {1.f, 1.f}},
	{{-0.5f, 1.f, 1.f}, {0.f, 1.f, 0.f}, {0.f, 0.f}},
	{{0.5f, 1.f, 1.f}, {0.f, 1.f, 0.f}, {1.f, 0.f}}
};

static std::vector<vertex> world_object_horizontal_mesh = {
	{{0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f}},
	{{1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f}},
	{{0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f}},
	{{1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f}}
};

buffer_pair create_world_object_buffer() {

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, world_object_vertical_mesh.size() * sizeof(vertex), world_object_vertical_mesh.data(), GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(0));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(sizeof(float) * 3));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(sizeof(float) * 3 + sizeof(float) * 3));

	return {vao, vbo};
}

buffer_pair create_world_tile_buffer() {

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, world_object_horizontal_mesh.size() * sizeof(vertex), world_object_horizontal_mesh.data(), GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(0));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(sizeof(float) * 3));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(sizeof(float) * 3 + sizeof(float) * 3));

	return {vao, vbo};
}

constexpr inline int grass_quads =512;

buffer_pair create_grass_buffer(
	std::default_random_engine& rng,
	std::uniform_real_distribution<float>& uniform
) {
	static std::vector<vertex> grass_mesh;

	for (int count = 0; count < grass_quads; count++) {
		grass_mesh.push_back(world_object_vertical_mesh[0]);
		grass_mesh.push_back(world_object_vertical_mesh[1]);
		grass_mesh.push_back(world_object_vertical_mesh[2]);
		grass_mesh.push_back(world_object_vertical_mesh[2]);
		grass_mesh.push_back(world_object_vertical_mesh[1]);
		grass_mesh.push_back(world_object_vertical_mesh[3]);

		auto shift = glm::vec3(uniform(rng), uniform(rng), 0.f);

		for (int prev = 0; prev < 6; prev++) {
			grass_mesh[grass_mesh.size() - 1 - prev].position += shift * GRASS_TILE_SIZE;
		}
	}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, grass_mesh.size() * sizeof(vertex), grass_mesh.data(), GL_STATIC_DRAW);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(0));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(sizeof(float) * 3));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex),  reinterpret_cast<void*>(sizeof(float) * 3 + sizeof(float) * 3));

	return {vao, vbo};
}

namespace geometry {

namespace screen_relative {
struct point {
	glm::vec2 data;
};
}

namespace world{
struct point {
	glm::vec2 data;
};
}

namespace screen_opengl {
struct point {
	glm::vec2 data;
};

point convert(const geometry::world::point in, const geometry::world::point camera, const float aspect_ratio, const float zoom) {
	auto result = (in.data - camera.data) * glm::vec2 { aspect_ratio, 1.f } * zoom;
	return {
		result
	};
}
}

}

struct shader_2d_data {
	GLuint shift;
	GLuint zoom;
	GLuint aspect_ratio;
};

constexpr inline int ACTION_LOGIN = 0;
struct action_update {
	int action;
	int player_id;
	int fighter_id;
	int entity_id;
};

struct position_update {
	int timestamp;
	int spatial_entity_id;
	float x;
	float y;
	float direction;
	float speed;

	int fighter_id;
	float energy;
	uint16_t hp;
	uint16_t max_hp;
};

namespace command {

inline constexpr uint8_t MOVE = 0;
inline constexpr uint8_t RUN_START = 1;
inline constexpr uint8_t RUN_STOP = 2;

struct data {
	int32_t player;
	int32_t target_entity;
	float target_x;
	float target_y;
	uint8_t command_type;
	uint8_t command_data;
	uint8_t padding[2];
};

}

static dcon::data_container container {};

int main(void)
{

	ankerl::unordered_dense::map<int, dcon::visible_spatial_entity_id> index_to_spatial_entity;
	ankerl::unordered_dense::map<int, dcon::known_fighter_id> index_to_fighter;
	std::optional<int> my_player_index;
	std::optional<int> my_fighter_index;
	std::optional<int> my_spatial_index;

	float my_energy;
	std::vector<float> energy_history;
	energy_history.resize(60 * 10);

	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform{0.0, 1.0};
	std::normal_distribution<float> normal_d{0.f, 0.1f};
	std::normal_distribution<float> size_d{1.f, 0.3f};


	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	GLFWwindow* window;
	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
	window = glfwCreateWindow(1280 * main_scale, 960 * main_scale, "3", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	// ImGui::StyleColorsDark();
	ImGui::StyleColorsLight();

	geometry::world::point camera { {0.f, 0.f} };
	float zoom = 0.1f;

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)


	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

	// GLEW validation
	if (auto result = glewInit(); result != GLEW_NO_ERROR)
		glew_fail("glewInit: ", result);
	if (!GLEW_VERSION_3_3)
		throw std::runtime_error("OpenGL 3.3 is not supported");

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	// illumination settings
	glm::vec3 light_color = glm::vec3(1.f, 0.8f, 0.3f);
	glm::vec3 ambient = glm::vec3(0.2f, 0.6f, 0.8f);
	glClearColor(ambient.x, ambient.y, ambient.z, 0.f);


	assert_no_errors();


	auto object_mesh = create_world_object_buffer();
	auto tile_mesh = create_world_tile_buffer();
	auto grass_mesh = create_grass_buffer(rng, uniform);

	load_textures();

	assert_no_errors();



	// setting up a flat shader
	std::string flat_shader_vertex_path = "./shaders/basic_shader_flat.vert";
	std::string flat_shader_fragment_path = "./shaders/basic_shader_flat.frag";
	std::string flat_vertex_shader_source = read_shader( flat_shader_vertex_path );
	std::string flat_fragment_shader_source = read_shader( flat_shader_fragment_path );
	auto flat_shader = create_program(
		create_shader(GL_VERTEX_SHADER, flat_vertex_shader_source.c_str()),
		create_shader(GL_FRAGMENT_SHADER, flat_fragment_shader_source.c_str())
	);
	GLuint flat_model_location = glGetUniformLocation(flat_shader, "model");
	GLuint flat_view_location = glGetUniformLocation(flat_shader, "view");
	GLuint flat_projection_location = glGetUniformLocation(flat_shader, "projection");
	GLuint flat_albedo_location = glGetUniformLocation(flat_shader, "albedo");


	// setting up a basic shader
	std::string basic_shader_vertex_path = "./shaders/basic_shader_meshes.vert";
	std::string basic_shader_fragment_path = "./shaders/basic_shader_meshes.frag";
	std::string vertex_shader_source = read_shader( basic_shader_vertex_path );
	std::string fragment_shader_source = read_shader( basic_shader_fragment_path );
	auto basic_shader = create_program(
		create_shader(GL_VERTEX_SHADER, vertex_shader_source.c_str()),
		create_shader(GL_FRAGMENT_SHADER, fragment_shader_source.c_str())
	);
	GLuint model_location = glGetUniformLocation(basic_shader, "model");
	GLuint view_location = glGetUniformLocation(basic_shader, "view");
	GLuint projection_location = glGetUniformLocation(basic_shader, "projection");
	GLuint albedo_texture_location = glGetUniformLocation(basic_shader, "albedo_texture");
	GLuint flip_location = glGetUniformLocation(basic_shader, "flip");

	assert_no_errors();

	float albedo_world[] = {0.4f, 0.5f, 0.8f};
	float albedo_character[] = {0.5f, 0.5f, 0.5f};
	float albedo_critter[] = {0.5f, 0.1f, 0.1f};

	glm::vec3 camera_position{0.f, 0.f, 15.f};
	glm::vec3 camera_target{0.f, 0.f, 15.f};

	int width = 1;
	int height = 1;

	assert_no_errors();

	float update_timer = 0.f;
	glm::vec3 light_direction {0.5f, 0.5f, 0.5f};

	glm::vec2 camera_speed = {};
	int tick = 0;
	float data[512] {};


	// Connection stuff
	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	addrinfo *result = NULL;
	addrinfo *ptr = NULL;

	addrinfo hints_tcp;
	ZeroMemory( &hints_tcp, sizeof(hints_tcp) );
	hints_tcp.ai_family   = AF_UNSPEC;
	hints_tcp.ai_socktype = SOCK_STREAM;
	hints_tcp.ai_protocol = IPPROTO_TCP;


	addrinfo hints_udp;
	ZeroMemory( &hints_udp, sizeof(hints_udp) );
	hints_udp.ai_family   = AF_UNSPEC;
	hints_udp.ai_socktype = SOCK_DGRAM;
	hints_udp.ai_protocol = IPPROTO_UDP;

	SOCKET ConnectSocketTCP = INVALID_SOCKET;
	SOCKET ConnectSocketUDP = INVALID_SOCKET;

	std::string connection_address = "127.0.0.1";
	int connection_port = 8080;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	action_update action_received {};
	position_update position_received {};

	bool udp_socket_ready = false;

	std::thread position_updates([&](){
		while(1) {
			if (!udp_socket_ready) {
				Sleep(10);
			}
			iResult = recv(ConnectSocketUDP, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				// printf("Bytes received: %d\n", iResult);
				memcpy(&position_received, recvbuf, sizeof(position_update));

				auto spatial = position_received.spatial_entity_id;

				dcon::visible_spatial_entity_id location_to_link{};
				dcon::known_fighter_id fighter_to_link{};

				auto it = index_to_spatial_entity.find(spatial);
				if (it != index_to_spatial_entity.end()) {
					auto entity = it->second;
					container.visible_spatial_entity_set_direction(entity, position_received.direction);

					auto last_x = container.visible_spatial_entity_get_x(entity);
					auto last_y = container.visible_spatial_entity_get_y(entity);

					auto distance = sqrtf((position_received.x - last_x) * (position_received.x - last_x) +  (position_received.y - last_y) * (position_received.y - last_y));
					auto path = container.visible_spatial_entity_get_path_length(entity);
					if (distance > 0.01f) {
						distance /= 3.f;
					}
					container.visible_spatial_entity_set_path_length(entity, path + distance);

					container.visible_spatial_entity_set_x(entity, position_received.x);
					container.visible_spatial_entity_set_y(entity, position_received.y);

					if (position_received.spatial_entity_id == my_spatial_index) {
						camera_target.x = position_received.x;
						camera_target.y = -position_received.y - 15.f;
					}
					location_to_link = entity;
				} else {
					auto entity = container.create_visible_spatial_entity();
					container.visible_spatial_entity_set_direction(entity, position_received.direction);
					container.visible_spatial_entity_set_path_length(entity, 0.f);
					container.visible_spatial_entity_set_x(entity, position_received.x);
					container.visible_spatial_entity_set_y(entity, position_received.y);

					index_to_spatial_entity[spatial] = entity;
					location_to_link = entity;
				}

				if (position_received.fighter_id > -1) {
					auto it2 = index_to_fighter.find(position_received.fighter_id);
					if (it2 != index_to_fighter.end()) {
						auto entity = it2->second;
						container.known_fighter_set_hp_current(entity, position_received.hp);
						container.known_fighter_set_hp_max(entity, position_received.max_hp);
						if (position_received.fighter_id == my_fighter_index) {
							my_energy = position_received.energy;
							for (int index = 0; index < energy_history.size() - 1; index++) {
								energy_history[index] = energy_history[index + 1];
							}
							energy_history.back() = my_energy;
						}
						fighter_to_link = entity;
					} else {
						auto entity = container.create_known_fighter();
						container.known_fighter_set_hp_current(entity, position_received.hp);
						container.known_fighter_set_hp_max(entity, position_received.max_hp);
						index_to_fighter[position_received.fighter_id] = entity;
						fighter_to_link = entity;
					}
				}

				if (fighter_to_link && location_to_link) {
					container.force_create_fighter_location(fighter_to_link, location_to_link);
				}

			}
			else if (iResult == 0) {
				printf("Connection closed\n");
			} else {
				// timeout: do nothing
			}
		}
	});

	double last_time = glfwGetTime();
	while (!glfwWindowShouldClose(window))
	{
		tick++;

		double time = glfwGetTime();
		float dt = (float)(time - last_time);
		last_time = time;

		update_timer += dt;

		if (update_timer > 1.f / 60.f) {
			update_timer = 0.f;
		}
		camera_speed = camera_target.xy - camera_position.xy;
		camera_position.xy += camera_speed * (1.f - exp(-dt * 10.f));

		// if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0) {
		// 	ImGui_ImplGlfw_Sleep(10);
		// 	continue;
		// }

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();


		const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

		glm::vec3 light_direction {cosf(time / 100.f), sinf(time / 100.f), 2.5f};
		glm::vec3 light_z = glm::normalize(light_direction);
		glm::vec3 light_x = glm::normalize(glm::cross(light_z, {0.f, 0.f, 1.f}));
		glm::vec3 light_y = glm::cross(light_x, light_z);

		{
			ImGui::Begin("Stats");
			if (ImPlot::BeginPlot("Energy")) {
				static ImPlotAxisFlags flags;
				ImPlot::SetupAxes("X","Y",flags,flags);
				ImPlot::SetupAxesLimits(0,energy_history.size(),0,1);
				ImPlot::SetupAxisZoomConstraints(ImAxis_Y1, 1.f, 1.f);
				ImPlot::PlotLine("Energy", energy_history.data(), energy_history.size());
				ImPlot::EndPlot();
			}
			ImGui::End();
		}

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Enter server");                          // Create a window called "Hello, world!" and append into it.

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::InputText("Server IP address", &connection_address);
			ImGui::InputInt("Port", &connection_port);
			if (ConnectSocketTCP == INVALID_SOCKET && ImGui::Button("Connect")) {
				iResult = getaddrinfo(
					connection_address.c_str(),
					std::to_string(connection_port).c_str(),
					&hints_tcp,
					&result
				);
				if (iResult != 0) {
					printf("getaddrinfo failed: %d\n", iResult);
					WSACleanup();
					return 1;
				}
				// Attempt to connect to the first address returned by
				// the call to getaddrinfo
				ptr=result;

				// Create a SOCKET for connecting to server
				ConnectSocketTCP = socket(
					ptr->ai_family,
					ptr->ai_socktype,
					ptr->ai_protocol
				);
				if (ConnectSocketTCP == INVALID_SOCKET) {
					printf("Error at socket(): %d\n", WSAGetLastError());
					freeaddrinfo(result);
					WSACleanup();
					return 1;
				}
				iResult = connect( ConnectSocketTCP, ptr->ai_addr, (int)ptr->ai_addrlen);
				if (iResult == SOCKET_ERROR) {
					closesocket(ConnectSocketTCP);
					ConnectSocketTCP = INVALID_SOCKET;
				}
				freeaddrinfo(result);
				if (ConnectSocketTCP == INVALID_SOCKET) {
					printf("Unable to connect to server!\n");
					WSACleanup();
					return 1;
				}

				printf("TCP Connected!\n");

				// Create a SOCKET for connecting to server
				ConnectSocketUDP = socket(
					AF_INET,
					SOCK_DGRAM,
					IPPROTO_UDP
				);
				if (ConnectSocketUDP == INVALID_SOCKET) {
					printf("Error at socket(): %d\n", WSAGetLastError());
					freeaddrinfo(result);
					WSACleanup();
					return 1;
				}
				printf("UDP Socket created!\n");

				int timeout = 1;
				setsockopt(ConnectSocketTCP, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
				// setsockopt(ConnectSocketUDP, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(int));
				u_long mode = 1;  // 1 to enable non-blocking socket
				// ioctlsocket(ConnectSocketUDP, FIONBIO, &mode);
				sockaddr_in dest;
				// sockaddr_in local;

				// local.sin_family = AF_INET;
				// inet_pton(AF_INET, "127.0.0.1", &local.sin_addr.s_addr);
				// local.sin_port = htons(0);

				dest.sin_family = AF_INET;
				inet_pton(AF_INET, connection_address.c_str(), &dest.sin_addr.s_addr);
				dest.sin_port = htons(connection_port);

				// auto iResult = bind(ConnectSocketUDP, (sockaddr *)&local, sizeof(local));
				if (iResult == SOCKET_ERROR) {
					closesocket(ConnectSocketTCP);
					ConnectSocketTCP = INVALID_SOCKET;
				}

				// sendto(s, pkt, strlen(pkt), 0, (sockaddr *)&dest, sizeof(dest));

				const char *sendbuf = "Subscribe";
				for (int count = 0; count < 4; count++) {
					iResult = sendto(ConnectSocketUDP, sendbuf, (int) strlen(sendbuf), 0,  (sockaddr *)&dest, sizeof(dest));
					if (iResult == SOCKET_ERROR) {
						printf("send failed: %d\n", WSAGetLastError());
						closesocket(ConnectSocketUDP);
						WSACleanup();
						return 1;
					}
					printf("Bytes Sent: %d\n", iResult);
				}
				udp_socket_ready = true;
			}

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		ImGui::Render();

		//
		if (ConnectSocketTCP != INVALID_SOCKET){
			iResult = recv(ConnectSocketTCP, recvbuf, recvbuflen, 0);
			if (iResult > 0) {
				printf("Bytes received: %d\n", iResult);
				memcpy(&action_received, recvbuf, sizeof(action_update));

				if (action_received.action == ACTION_LOGIN) {
					my_fighter_index = action_received.fighter_id;
					my_spatial_index = action_received.entity_id;
					my_player_index = action_received.player_id;
					printf("I am %d at location %d", my_fighter_index.value(), my_spatial_index.value());
				}
			}
			else if (iResult == 0) {
				printf("Connection closed\n");
			} else {
				// timeout: do nothing
			}

			if (my_player_index && (prev_move_x != current_move_x || prev_move_y != current_move_y)) {

				command::data data {};
				data.player = my_player_index.value();
				data.command_type = 0;
				data.target_x = current_move_x;
				data.target_y = current_move_y;

				prev_move_x = current_move_x;
				prev_move_y = current_move_y;

				iResult = send(ConnectSocketTCP, (char*)&data, (int) sizeof(command::data), 0);
				if (iResult == SOCKET_ERROR) {
					printf("send failed: %d\n", WSAGetLastError());
					closesocket(ConnectSocketTCP);
					WSACleanup();
					return 1;
				}
			}

			if (my_player_index && (prev_running != running)) {

				command::data data {};
				data.player = my_player_index.value();
				if (running) {
					data.command_type = command::RUN_START;
				} else {
					data.command_type = command::RUN_STOP;
				}
				prev_running = running;

				iResult = send(ConnectSocketTCP, (char*)&data, (int) sizeof(command::data), 0);
				if (iResult == SOCKET_ERROR) {
					printf("send failed: %d\n", WSAGetLastError());
					closesocket(ConnectSocketTCP);
					WSACleanup();
					return 1;
				}
			}
		}


		float near_plane = 0.1f;
		float far_plane = 80.f;
		glm::mat4 view(1.f);
		view = glm::rotate(view, -glm::pi<float>() / 4.f, {1.f, 0.f, 0.f});
		view = glm::translate(view, -camera_position);
		// drawing shadow maps
		std::vector<glm::mat4> shadow_projections;
		glm::mat4 projection_full_range = glm::perspective(
			glm::pi<float>() / 4.f, (1.f * width) / height, near_plane, far_plane
		);

		assert_no_errors();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glfwGetFramebufferSize(window, &width, &height);

		assert_no_errors();

		width = std::max(width, 10);
		height = std::max(height, 10);

		glViewport(0, 0, width, height);
		float aspect_ratio = (float) width / (float) height;
		glClearColor(ambient.x, ambient.y, ambient.z, 0.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		assert_no_errors();


		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// glDisable(GL_CULL_FACE);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		assert_no_errors();

		glUseProgram(basic_shader);
		glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
		glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection_full_range));


		/*
		Start very simple, optimise later:
		Render tiles as horisontal squares.
		Render grass as vertical squares.
		*/

		glBindVertexArray(tile_mesh.vao);

		auto visible_tile_x = floor(camera_position.x / TILE_SIZE);
		auto visible_tile_y = floor(-camera_position.y / TILE_SIZE);

		for (float i = visible_tile_x - 2.f; i <= visible_tile_x + 2.f; i++) {
			for (float j = visible_tile_y-1.f;  j <= visible_tile_y + 1.f; j++) {
				glm::mat4 model (1.f);
				model = glm::translate(model, {i * TILE_SIZE, -j * TILE_SIZE, 0.f});
				// glUniform3fv(albedo_location, 1, albedo_character);

				choose_tile_texture(albedo_texture_location, flip_location);

				model = glm::scale(model, {TILE_SIZE, TILE_SIZE, 1.f});
				glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
				glDrawArrays(
					GL_TRIANGLE_STRIP,
					0,
					4
				);
			}
		}


		auto visible_grass_tile_x = floor(camera_position.x / GRASS_TILE_SIZE);
		auto visible_grass_tile_y = floor(-camera_position.y / GRASS_TILE_SIZE);

		glBindVertexArray(grass_mesh.vao);
		for (float i = visible_grass_tile_x - 3.f; i <= visible_grass_tile_x + 1.f; i++) {
			for (float j = visible_grass_tile_y - 3.f; j <= visible_grass_tile_y; j++) {
				glm::mat4 model (1.f);
				// glUniform3fv(albedo_location, 1, albedo_character);


				auto seed = ((int)i ^ (int)j);

				rng.seed(seed);

				for (int count = 0; count < 20; count++) {

					choose_grass_texture(albedo_texture_location, flip_location, count);

					auto dx = uniform(rng);
					auto dy = uniform(rng);
					auto size = 0.5f  + uniform(rng) * 0.5f;

					glm::mat4 model (1.f);
					model = glm::translate(model, {(i + dx) * GRASS_TILE_SIZE, -(j + dy) * GRASS_TILE_SIZE, 0.01f});
					model = glm::scale(model, {1.0f, size, size});
					glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
					glDrawArrays(
						GL_TRIANGLES,
						0,
						6 * grass_quads
					);
				}
			}
		}

		glBindVertexArray(object_mesh.vao);
		for (float i = visible_tile_x - 2.f; i <= visible_tile_x + 1.f; i++) {
			for (float j = visible_tile_y - 1.f; j <= visible_tile_y + 1.f; j++) {
				glm::mat4 model (1.f);
				// glUniform3fv(albedo_location, 1, albedo_character);


				auto seed = ((int)i ^ (int)j ^ 546441212);

				rng.seed(seed);

				for (int count = 0; count < 50; count++) {

					choose_tree_texture(albedo_texture_location, flip_location, count);

					auto dx = uniform(rng);
					auto dy = uniform(rng);
					auto size = 3.f + uniform(rng);

					glm::mat4 model (1.f);
					model = glm::translate(model, {(i + dx) * TILE_SIZE, -(j + dy) * TILE_SIZE, 0.01f});
					model = glm::scale(model, {size, size, size});
					glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
					glDrawArrays(
						GL_TRIANGLE_STRIP,
						0,
						4
					);
				}
			}
		}

		glBindVertexArray(object_mesh.vao);
		for (float i = visible_tile_x - 2.f; i <= visible_tile_x + 1.f; i++) {
			for (float j = visible_tile_y - 1.f; j <= visible_tile_y + 1.f; j++) {
				glm::mat4 model (1.f);
				// glUniform3fv(albedo_location, 1, albedo_character);


				auto seed = ((int)i ^ (int)j ^ 3451231);

				rng.seed(seed);

				for (int count = 0; count < 200; count++) {

					choose_bush_texture(albedo_texture_location, flip_location, count);

					auto dx = uniform(rng);
					auto dy = uniform(rng);
					auto size = 2.f + uniform(rng) * 0.5f;

					glm::mat4 model (1.f);
					model = glm::translate(model, {(i + dx) * TILE_SIZE, -(j + dy) * TILE_SIZE, 0.01f});
					model = glm::scale(model, {size, size, size});
					glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
					glDrawArrays(
						GL_TRIANGLE_STRIP,
						0,
						4
					);
				}
			}
		}

		assert_no_errors();

		glBindVertexArray(object_mesh.vao);
		container.for_each_visible_spatial_entity([&] (dcon::visible_spatial_entity_id cid) {
			auto model_tag = container.visible_spatial_entity_get_model(cid);
			auto size = 1.f;
			if (model_tag == 0) {
				choose_rat_sprite(
					albedo_texture_location,
					flip_location,
					container.visible_spatial_entity_get_direction(cid),
					container.visible_spatial_entity_get_path_length(cid),
					false
				);
				size = 2.f;
			} else  {
				choose_rogue_sprite(
					albedo_texture_location,
					flip_location,
					container.visible_spatial_entity_get_direction(cid),
					container.visible_spatial_entity_get_path_length(cid),
					false
				);
			}

			glm::mat4 model (1.f);
			model = glm::translate(model, {container.visible_spatial_entity_get_x(cid), -container.visible_spatial_entity_get_y(cid), 0.01f});
			model = glm::scale(model, {size, size, size});
			glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
			glDrawArrays(
				GL_TRIANGLE_STRIP,
				0,
				4
			);
		});

		// draw UI elements on top of characters
		glUseProgram(flat_shader);
		glUniformMatrix4fv(flat_view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
		glUniformMatrix4fv(flat_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection_full_range));

		glBindVertexArray(object_mesh.vao);
		container.for_each_visible_spatial_entity([&] (dcon::visible_spatial_entity_id cid) {
			auto fighter = container.visible_spatial_entity_get_fighter_from_fighter_location(cid);
			if (!fighter) {
				return;
			}

			auto model_tag = container.visible_spatial_entity_get_model(cid);
			auto size = 1.f;
			if (model_tag == 0) {
				size = 2.f;
			} else  {
			}

			auto hp = container.known_fighter_get_hp_current(fighter);
			auto max_hp = container.known_fighter_get_hp_max(fighter);
			auto width = float(hp) / float(max_hp);
			auto shift = - width / 2.f;

			//bg
			{
				glm::mat4 model (1.f);
				model = glm::translate(model, {container.visible_spatial_entity_get_x(cid) - 0.5f, -container.visible_spatial_entity_get_y(cid) + size, 0.f + size});
				model = glm::rotate(model, 0.3f, {0.f, 0.f, 1.f});
				model = glm::scale(model, {0.5f, 0.1f, 0.1f});
				glUniformMatrix4fv(flat_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
				glUniform4f(flat_albedo_location, 0.1f, 0.1f, 0.1f, 1.f);
				glDrawArrays(
					GL_TRIANGLE_STRIP,
					0,
					4
				);
			}

			// top
			if (max_hp > 0) {
				glm::mat4 model (1.f);
				model = glm::translate(model, {container.visible_spatial_entity_get_x(cid) - 0.5f, -container.visible_spatial_entity_get_y(cid) + size, 0.05f + size});
				model = glm::rotate(model, 0.3f, {0.f, 0.f, 1.f});
				model = glm::translate(model, {-0.5 *(1.f - width), 0.f, 0.f});
				model = glm::scale(model, {0.5f *width, 0.1f, 0.1f});

				glUniformMatrix4fv(flat_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
				glUniform4f(flat_albedo_location, 0.8f, 0.8f, 0.9f, 1.f);
				glDrawArrays(
					GL_TRIANGLE_STRIP,
					0,
					4
				);
			}
		});

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
		assert_no_errors();
	}

	// Cleanup/

	closesocket(ConnectSocketTCP);
	closesocket(ConnectSocketUDP);
	WSACleanup();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}