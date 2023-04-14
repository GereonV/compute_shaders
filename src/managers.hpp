#ifndef CS_MANAGERS_HPP
#define CS_MANAGERS_HPP

#include <numbers>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader.hpp"

enum class shader_image_access : GLenum {
	read_only = GL_READ_ONLY,
	write_only = GL_WRITE_ONLY,
	read_write = GL_READ_WRITE,
};

class texture_manager {
public:
	// !width || !height causes UB
	texture_manager(GLsizei width, GLsizei height) noexcept;
	texture_manager(texture_manager const &) = delete;
	~texture_manager();
	// !width || !height causes UB
	void reset(GLsizei width, GLsizei height) noexcept;
	void clear() const noexcept;
	void bind_to_texture_unit(GLuint unit) const noexcept;
	void bind_to_image_unit(GLuint unit, shader_image_access access) const noexcept;
private:
	GLuint _texture; // GL_TEXTURE_IMMUTABLE_FORMAT
};

namespace slime {
	struct agent {
		float x, y; // ∈ [0, 1]
		float angle_radians;
		unsigned int species; // ∈ [0, 3]
	};

	struct species_settings {
		float color[4]{}; // only first 3 used
		float move_speed{0.15f};
		float turn_radians_per_second{std::numbers::pi_v<float> / 2};
		float sensor_spacing_radians{std::numbers::pi_v<float> / 6.0f};
		float sensor_distance{0.015f};
	};

	class manager {
	public:
		using species_array = species_settings[4];
		manager(GLuint num_agents, GLsizei width, GLsizei height) noexcept;
		manager(manager const &) = delete;
		void compute() const noexcept;
		// returns whether size changed
		[[nodiscard]] bool resize(GLsizei width, GLsizei height) noexcept;
		// returns false when it has been closed
		[[nodiscard]] bool draw_settings_window(GLuint max_num_agents) noexcept;
		[[nodiscard]] species_array       & species()       noexcept;
		[[nodiscard]] species_array const & species() const noexcept;
		void num_agents(GLuint num_agents) noexcept;
		[[nodiscard]] GLuint num_agents() const noexcept;
		void decay_rate(float decay_rate) noexcept;
		[[nodiscard]] float decay_rate() const noexcept;
		void diffuse_rate(float diffuse_rate) noexcept;
		[[nodiscard]] float diffuse_rate() const noexcept;
	private:
		species_array _species;
		GLuint _num_agents;
		float _decay_rate, _diffuse_rate;
		mutable float _last_time;
		GLsizei _width, _height;
		shader_program _simulation_shader, _postprocess_shader;
		texture_manager _trail_manager, _colored_manager;
	};

	[[nodiscard]] manager make_manager(GLuint num_agents, GLFWwindow * window = glfwGetCurrentContext()) noexcept;
	[[nodiscard]] bool resize(manager & manager, GLFWwindow * window = glfwGetCurrentContext()) noexcept;
	void set_colors_to_default(manager & manager) noexcept;
}

#endif // CS_MANAGERS_HPP
