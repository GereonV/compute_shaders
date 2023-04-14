#include "managers.hpp"
#include <cmath>
#include <iostream>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include "shadersrc.hpp"

inline bool draw_uint(char const * label, GLuint & value, unsigned int step, int max) noexcept {
	int x = static_cast<int>(value);
	auto r = ImGui::DragInt(label, &x, static_cast<float>(step), 0, max, nullptr, ImGuiSliderFlags_AlwaysClamp);
	value = x;
	return r;
}

inline bool draw_float(char const * label, float & value, float max) noexcept {
	return ImGui::DragFloat(label, &value, max / 100.0f, 0.0f, max, nullptr, ImGuiSliderFlags_AlwaysClamp);
}

inline bool draw_positive_float(char const * label, float & value, float step) noexcept {
	return ImGui::DragFloat(label, &value, step, 0.0f, FLT_MAX, nullptr, ImGuiSliderFlags_AlwaysClamp);
}

inline bool draw_symmetric_float(char const * label, float & value, float abs) noexcept {
	return ImGui::DragFloat(label, &value, abs / 50.0f, -abs, abs, nullptr, ImGuiSliderFlags_AlwaysClamp);
}

inline void draw_species_text(unsigned char index) noexcept {
	static constinit char text[]{"Species x:"};
	text[sizeof(text) - 3] = static_cast<char>(index + '1');
	ImGui::Text(text);
}

inline void draw_species(slime::species_settings & species) noexcept {
	ImGui::ColorEdit3("Color", species.color, ImGuiColorEditFlags_NoDragDrop);
	draw_float("Move Speed", species.move_speed, 0.5f);
	draw_symmetric_float("Turn Speed", species.turn_radians_per_second, std::numbers::pi_v<float> / 2.0f);
	draw_float("Sensor Spacing", species.sensor_spacing_radians, std::numbers::pi_v<float> / 2.0f);
	draw_float("Sensor Distance", species.sensor_distance, 0.05f);
}

bool slime::manager::draw_settings_window(GLuint max_num_agents) noexcept {
	auto open = true;
	if(ImGui::Begin("Settings", &open)) {
		ImGui::Text("General");
		draw_uint("Number of Agents", _num_agents, 100, max_num_agents);
		draw_positive_float("Decay Rate", _decay_rate, 0.01f);
		draw_positive_float("Diffuse Rate", _diffuse_rate, 0.05f);
		for(unsigned char i{}; i < 4; ++i) {
			ImGui::Separator();
			draw_species_text(i);
			ImGui::PushID(i);
			draw_species(_species[i]);
			ImGui::PopID();
		}
	}
	ImGui::End();
	return open;
}

inline void reset_texture_managers(texture_manager & trail, texture_manager & colored) noexcept {
	trail.clear();
	trail.bind_to_image_unit(0, shader_image_access::read_write);
	colored.bind_to_image_unit(1, shader_image_access::write_only);
	colored.bind_to_texture_unit(0);
}


static shader_program_builder simulation_builder, postprocess_builder; // inline?

slime::manager::manager(GLuint num_agents, GLsizei width, GLsizei height) noexcept
	: _num_agents{num_agents},
	  _decay_rate{0.1f},
	  _diffuse_rate{3.0f},
	  _last_time{static_cast<float>(glfwGetTime())},
	  _width{width},
	  _height{height},
	  _simulation_shader{simulation_builder.build(SLIME_GLSL)},
	  _postprocess_shader{postprocess_builder.build(POSTPROCESS_GLSL)},
	  _trail_manager{width, height},
	  _colored_manager{width, height} {
#ifdef _DEBUG
	std::cerr << simulation_builder.error_message();
	std::cerr << postprocess_builder.error_message();
	if(simulation_builder.error())
		throw simulation_builder.error_message();
	if(postprocess_builder.error())
		throw postprocess_builder.error_message();
#endif // _DEBUG
	reset_texture_managers(_trail_manager, _colored_manager);
}

bool slime::manager::resize(GLsizei width, GLsizei height) noexcept {
	if(_width == width && _height == height)
		return false;
	_width = width;
	_height = height;
	_trail_manager.reset(width, height);
	_colored_manager.reset(width, height);
	reset_texture_managers(_trail_manager, _colored_manager);
	return true;
}

void slime::manager::compute() const noexcept {
	auto time = static_cast<float>(glfwGetTime());
	auto delta_time = time - _last_time;
	_last_time = time;
	_simulation_shader.use_program();
	glUniform1f(0, time);
	glUniform1f(1, delta_time);
	glUniform1ui(2, _num_agents);
	for(GLint location{3}; auto const & species : _species) {
		glUniform1f(location++, species.move_speed);
		glUniform1f(location++, species.turn_radians_per_second);
		glUniform1f(location++, species.sensor_spacing_radians);
		glUniform1f(location++, species.sensor_distance);
	}
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glDispatchCompute(_num_agents, 1, 1);
	_postprocess_shader.use_program();
	glUniform1f(0, delta_time);
	glUniform1f(1, _decay_rate);
	glUniform1f(2, _diffuse_rate);
	for(GLint location{3}; auto const & species : _species)
		glUniform3fv(location++, 1, species.color);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glDispatchCompute(_width / 32 + 1, _height / 32 + 1, 1);
}

void slime::manager::clear() const noexcept {
	_trail_manager.clear();
}

slime::manager::species_array & slime::manager::species() noexcept {
	return _species;
}

slime::manager::species_array const & slime::manager::species() const noexcept {
	return _species;
}

void slime::manager::num_agents(GLuint num_agents) noexcept {
	_num_agents = static_cast<int>(num_agents);
}

GLuint slime::manager::num_agents() const noexcept {
	return _num_agents;
}

void slime::manager::decay_rate(float decay_rate) noexcept {
	_decay_rate = decay_rate;
}

float slime::manager::decay_rate() const noexcept {
	return _decay_rate;
}

void slime::manager::diffuse_rate(float diffuse_rate) noexcept {
	_diffuse_rate = diffuse_rate;
}

float slime::manager::diffuse_rate() const noexcept {
	return _diffuse_rate;
}

slime::manager slime::make_manager(GLuint num_agents, GLFWwindow * window) noexcept {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return {num_agents, width, height};
}

bool slime::resize(slime::manager & manager, GLFWwindow * window) noexcept {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return manager.resize(width, height);
}

void slime::set_colors_to_default(manager & manager) noexcept {
	manager.species()[0].color[0] = 1.0f;
	manager.species()[1].color[1] = 1.0f;
	manager.species()[2].color[2] = 1.0f;
}

inline std::uniform_real_distribution<float> dist01;
inline std::uniform_real_distribution<float> dist_abs1{-1.0f, 1.0f};
inline std::uniform_real_distribution<float> angle_dist{0.0f, 2.0f * std::numbers::pi_v<float>};

void slime::randomly_setup(agent * agents, GLuint num_agents, unsigned char num_species, std::mt19937 & twister) noexcept {
	std::uniform_int_distribution<int> species_dist{0, num_species - 1};
	for(GLuint i{}; i < num_agents; ++i) {
		auto & agent = agents[i];
		agent.x = dist01(twister);
		agent.y = dist01(twister);
		agent.angle_radians = angle_dist(twister);
		agent.species = species_dist(twister);
	}
}

void slime::randomly_setup_circle(agent * agents, GLuint num_agents, unsigned char num_species, std::mt19937 & twister) noexcept {
	std::uniform_int_distribution<int> species_dist{0, num_species - 1};
	for(GLuint i{}; i < num_agents; ++i) {
		auto & agent = agents[i];
		float x, y;
		do {
			x = dist_abs1(twister);
			y = dist_abs1(twister);
		} while(x * x + y * y > 1);
		agent.x = x / 2.0f + 0.5f;
		agent.y = y / 2.0f + 0.5f;
		agent.angle_radians = std::atan2(-y, -x);
		agent.species = species_dist(twister);
	}
}

void slime::randomize_species(agent * agents, GLuint num_agents, unsigned char num_species, std::mt19937 & twister) noexcept {
	std::uniform_int_distribution<int> species_dist{0, num_species - 1};
	for(GLuint i{}; i < num_agents; ++i)
		agents[i].species = species_dist(twister);
}
