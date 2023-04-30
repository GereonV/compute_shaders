#include "managers.hpp"
#include <cmath>
#include <numbers>
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "app.hpp"
#include "shader.hpp"
#include "shadersrc.hpp"


struct agent {
	float x, y; // pixels
	float angle_radians;
	unsigned int species; // ∈ [0, 3]
};

struct species_t {
	float color[4]; // only first 3 used
	float move_speed{0.15f};
	float turn_radians_per_second{std::numbers::pi_v<float> / 3.0f};
	float sensor_spacing_radians{std::numbers::pi_v<float> / 6.0f};
	float sensor_distance{0.015f};
};

static constinit agent agents[1'000'000]{};
static constinit GLuint max_num_agents{sizeof(agents) / sizeof(*agents)};
static constinit species_t species[4]{{{1.0f, 1.0f, 1.0f}}};

static bool menu_open;
static GLuint num_agents;
static float decay_rate, diffuse_rate;
static bool overlapping;
static GLuint num_species;
static void (* setup_function)() noexcept;

static std::mt19937 twister;
static GLsizei width, height;
static float last_time;

static GLuint ssbo;
static GLuint textures[2];
// static constexpr auto & [trail_texture, colored_texture] = textures;
static constexpr auto & trail_texture = textures[0];
static constexpr auto & colored_texture = textures[1];
static GLuint simulation_program, postprocess_program;

static void create_textures() noexcept {
	glCreateTextures(GL_TEXTURE_2D, 2, textures);
	glTextureStorage2D(trail_texture, 1, GL_RGBA32F, width, height);
	glTextureStorage2D(colored_texture, 1, GL_RGBA32F, width, height);
	glClearTexImage(trail_texture, 0, GL_RGBA, GL_FLOAT, nullptr);
	//                                    layered layer            shader store format
	glBindImageTexture(0, trail_texture, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindImageTexture(1, colored_texture, 0, false, 0, GL_WRITE_ONLY, GL_RGBA32F);
	glBindTextureUnit(0, colored_texture);
}

static void setup_uniform() noexcept {
	std::uniform_real_distribution<float>
		xdist{0.0f, static_cast<float>(width)},
		ydist{0.0f, static_cast<float>(height)},
		angledist{0.0f, 2.0f * std::numbers::pi_v<float>};
	for(GLuint i{}; i < max_num_agents; ++i) {
		auto & agent = agents[i];
		agent.x = xdist(twister);
		agent.y = ydist(twister);
		agent.angle_radians = angledist(twister);
	}
}

static void setup_circle() noexcept {
	std::uniform_real_distribution<float> dist{-1.0f, 1.0f};
	auto half_width = static_cast<float>(width) / 2.0f;
	auto half_height = static_cast<float>(height) / 2.0f;
	auto width_mul = half_width;
	auto height_mul = half_height;
	if(width > height)
		width_mul *= half_height / half_width;
	else
		height_mul *= half_width / half_height;
	for(GLuint i{}; i < max_num_agents; ++i) {
		float x, y;
		do {
			x = dist(twister);
			y = dist(twister);
		} while(x * x + y * y > 1);
		auto & agent = agents[i];
		agent.x = x * width_mul + half_width;
		agent.y = y * height_mul + half_height;
		agent.angle_radians = std::atan2(-y, -x);
	}
}

static bool draw_uint(char const * label, GLuint & value, unsigned int step, int max) noexcept {
	int x = static_cast<int>(value);
	auto r = ImGui::DragInt(label, &x, static_cast<float>(step), 0, max, nullptr, ImGuiSliderFlags_AlwaysClamp);
	value = x;
	return r;
}

static bool draw_float(char const * label, float & value, float max) noexcept {
	return ImGui::DragFloat(label, &value, max / 100.0f, 0.0f, max, nullptr, ImGuiSliderFlags_AlwaysClamp);
}

static bool draw_positive_float(char const * label, float & value, float step) noexcept {
	return ImGui::DragFloat(label, &value, step, 0.0f, FLT_MAX, nullptr, ImGuiSliderFlags_AlwaysClamp);
}

static bool draw_symmetric_float(char const * label, float & value, float abs) noexcept {
	return ImGui::DragFloat(label, &value, abs / 50.0f, -abs, abs, nullptr, ImGuiSliderFlags_AlwaysClamp);
}

// return whether number of species changed
[[nodiscard]] static bool imgui() noexcept {
	static constinit char text[]{"Species x:"};
	if(ImGui::IsKeyPressed(ImGuiKey_S, false))
		menu_open = !menu_open;
	if(!menu_open)
		return false;
	bool species_changed;
	if(ImGui::Begin("Settings", &menu_open)) {
		ImGui::Text("General");
		draw_uint("Number of Agents", num_agents, 100, max_num_agents);
		draw_positive_float("Decay Rate", decay_rate, 0.01f);
		draw_positive_float("Diffuse Rate", diffuse_rate, 0.05f);
		ImGui::Checkbox("Overlapping", &overlapping);
		species_changed = draw_uint("Number of Species", num_species, 1, 4);
		for(unsigned char i{}; i < num_species; ++i) {
			ImGui::Separator();
			ImGui::PushID(i);
			auto & s = species[i];
			text[sizeof(text) - 3] = static_cast<char>(i + '1');
			ImGui::Text(text);
			ImGui::ColorEdit3("Color", s.color, ImGuiColorEditFlags_NoDragDrop);
			draw_float("Move Speed", s.move_speed, 0.5f);
			draw_symmetric_float("Turn Speed", s.turn_radians_per_second, std::numbers::pi_v<float> / 2.0f);
			draw_float("Sensor Spacing", s.sensor_spacing_radians, std::numbers::pi_v<float> / 2.0f);
			draw_float("Sensor Distance", s.sensor_distance, 0.05f);
			ImGui::PopID();
		}
	}
	ImGui::End();
	if(species_changed) {
		std::uniform_int_distribution<int> dist{0, static_cast<int>(num_species)};
		for(GLuint i{}; i < max_num_agents; ++i)
			agents[i].species = dist(twister);
	}
	return species_changed;
}

// returns whether reset occured
[[nodiscard]] static bool prepare() noexcept {
	auto sub_needed = imgui();
	auto [width, height] = framebuffer_size();
	if(::width != width || ::height != height) {
		::width = width;
		::height = height;
		glDeleteTextures(2, textures);
		create_textures();
	} else {
		if(ImGui::IsKeyPressed(ImGuiKey_C, false))
			setup_function = setup_circle;
		else if(ImGui::IsKeyPressed(ImGuiKey_V, false))
			setup_function = setup_uniform;
		else if(!sub_needed && !ImGui::IsKeyPressed(ImGuiKey_R, false))
			return false;
		glClearTexImage(trail_texture, 0, GL_RGBA, GL_FLOAT, nullptr);
	}
	setup_function();
	glNamedBufferSubData(ssbo, 0, sizeof(agents), agents);
	return true;
}

void slime::init() noexcept {
	menu_open = false;
	num_agents = max_num_agents / 10;
	decay_rate = 0.1f;
	diffuse_rate = 3.0f;
	overlapping = false;
	num_species = 1;
	auto [width, height] = framebuffer_size();
	::width = width;
	::height = height;
	::last_time = static_cast<float>(glfwGetTime());
	(setup_function = setup_uniform)();
	glGenBuffers(1, &ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glNamedBufferData(ssbo, sizeof(agents), agents, GL_DYNAMIC_COPY); // TODO rethink usage
	create_textures();
	simulation_program = make_program(SLIME_GLSL);
	postprocess_program = make_program(POSTPROCESS_GLSL);
}

void slime::shutdown() noexcept {
	glDeleteBuffers(1, &ssbo);
	glDeleteTextures(2, textures);
	glDeleteProgram(simulation_program);
	glDeleteProgram(postprocess_program);
}

void slime::compute() noexcept {
	auto time = static_cast<float>(glfwGetTime());
	auto delta_time = prepare() ? 0.0f : time - last_time;
	last_time = time;
	glUseProgram(simulation_program);
	glUniform1f(0, time);
	glUniform1f(1, delta_time);
	glUniform1ui(2, num_agents);
	glUniform1ui(3, overlapping);
	auto mul = static_cast<float>(width);
	for(GLint location{4}; auto const & s : ::species) {
		glUniform1f(location++, s.move_speed * mul);
		glUniform1f(location++, s.turn_radians_per_second);
		glUniform1f(location++, s.sensor_spacing_radians);
		glUniform1f(location++, s.sensor_distance * mul);
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glDispatchCompute(num_agents, 1, 1);
	glUseProgram(postprocess_program);
	glUniform1f(0, delta_time);
	glUniform1f(1, decay_rate);
	glUniform1f(2, diffuse_rate);
	for(GLint location{3}; auto const & s : ::species)
		glUniform3fv(location++, 1, s.color);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glDispatchCompute(width / 32 + 1, height / 32 + 1, 1);
}
