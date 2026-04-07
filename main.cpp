#include "data_ids.hpp"
#define GLM_FORCE_SWIZZLE
#define GLEW_STATIC

#include <array>
#include <assert.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>

#include "stb_image/stb_image.h"


// Include all GLM core / GLSL features
#include <glm/glm.hpp> // vec2, vec3, mat4, radians
// Include all GLM extensions
#include <glm/ext.hpp> // perspective, translate, rotate
#include "glm/ext/matrix_transform.hpp"

#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"

#include "data.hpp"

#include "frustum.hpp"

#include "sprite.hpp"


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

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (key == GLFW_KEY_UP) {
		if (action == GLFW_PRESS) {
			current_move_y += 1;
		} else if (action == GLFW_RELEASE) {
			current_move_y -= 1;
		}
	}

	if (key == GLFW_KEY_DOWN) {
		if (action == GLFW_PRESS) {
			current_move_y -= 1;
		} else if (action == GLFW_RELEASE) {
			current_move_y += 1;
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
}

struct vertex {
	glm::vec3 position;
	glm::vec3 normal_direction;
	glm::vec2 texture_coordinate;
};

static std::vector<vertex> world_object_mesh = {
	{{-0.5f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f}},
	{{0.5f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f}},
	{{-0.5f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f}},
	{{0.5f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f}}
};

buffer_pair create_world_object_buffer() {

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, world_object_mesh.size() * sizeof(vertex), world_object_mesh.data(), GL_STATIC_DRAW);

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

static dcon::data_container container {};

int main(void)
{
	container.create_thing();

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

	load_rogue_textures();

	assert_no_errors();

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

	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform{0.0, 1.0};
	std::normal_distribution<float> normal_d{0.f, 0.1f};
	std::normal_distribution<float> size_d{1.f, 0.3f};

	glm::vec3 camera_position{0.f, 0.f, 15.f};

	int width = 1;
	int height = 1;

	assert_no_errors();

	float update_timer = 0.f;
	glm::vec3 light_direction {0.5f, 0.5f, 0.5f};

	glm::vec2 camera_speed = {};
	int tick = 0;
	float data[512] {};

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

		camera_speed *= exp(-dt * 10.f);
		camera_speed += glm::vec2(float(current_move_x), float(current_move_y)) * dt;

		camera_position.xy += camera_speed;

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
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("Test.");               // Display some text (you can use a format strings too)

			ImGui::SliderFloat3("float", (float*)&light_direction, -1.0f, 1.0f);
			light_direction = glm::normalize(light_direction);

			ImGui::ColorEdit3("clear color", (float*)&ambient); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		ImGui::Render();


		float near_plane = 0.1f;
		float far_plane = 20.f;
		glm::mat4 view(1.f);
		view = glm::rotate(view, -glm::pi<float>() / 9.f * 0.9f, {1.f, 0.f, 0.f});
		view = glm::translate(view, -camera_position);
		// drawing shadow maps
		std::vector<glm::mat4> shadow_projections;
		glm::mat4 projection_full_range = glm::perspective(
			glm::pi<float>() / 3.f, (1.f * width) / height, near_plane, far_plane
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

		assert_no_errors();

		glm::mat4 model (1.f);
		glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
		glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
		glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection_full_range));

		// glUniform3fv(albedo_location, 1, albedo_world);

		assert_no_errors();

		glBindVertexArray(object_mesh.vao);
		container.for_each_thing([&] (dcon::thing_id cid) {
			// auto rotation = container.thing_get_direction(cid);

			float rotation = time;
			auto dx = cosf(time);
			auto dy = sinf(time);

			container.thing_set_x(cid, container.thing_get_x(cid) + dx * dt);
			container.thing_set_y(cid, container.thing_get_y(cid) + dy * dt);
			container.thing_set_path_length(cid, container.thing_get_path_length(cid) + dt);

			choose_rogue_sprite(
				albedo_texture_location,
				flip_location,
				rotation,
				container.thing_get_path_length(cid),
				false
			);

			glm::mat4 model (1.f);
			model = glm::translate(model, {container.thing_get_x(cid), -container.thing_get_y(cid), 0.f});
			// glUniform3fv(albedo_location, 1, albedo_character);

			model = glm::scale(model, {1.f, 1.f, 1.f});
			// model = glm::rotate(model, rotation, glm::vec3{0.f, 0.f, 1.f});
			glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
			glDrawArrays(
				GL_TRIANGLE_STRIP,
				0,
				4
			);
		});


		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
		assert_no_errors();
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);

	glfwTerminate();
	return 0;
}