#ifndef CS_MANAGERS_HPP
#define CS_MANAGERS_HPP

#include <numbers>
#include <glad/glad.h>

namespace slime {
	struct agent {
		float x, y; // pixels
		float angle_radians;
		unsigned int species; // ∈ [0, 3]
	};

	struct species {
		float color[4]{}; // only first 3 used
		float move_speed{0.15f};
		float turn_radians_per_second{std::numbers::pi_v<float> / 3.0f};
		float sensor_spacing_radians{std::numbers::pi_v<float> / 6.0f};
		float sensor_distance{0.015f};
	};

	void init(agent * agents, GLuint max_num_agents, GLsizei width, GLsizei height, void (*setup_function)() noexcept) noexcept;
	void compute() noexcept;
	void reset() noexcept;
	void setup_uniform() noexcept;
	void setup_circle() noexcept;
	[[nodiscard]] bool resize_and_setup_if_needed(GLsizei width, GLsizei height, void (*setup_function)() noexcept) noexcept;
	[[nodiscard]] bool draw_settings_window(bool & open) noexcept; // returns whether agents were updated

	template<GLuint N>
	void init(agent (&agents)[N], GLsizei width, GLsizei height, void (*setup_function)() noexcept) noexcept {
		init(agents, N, width, height, setup_function);
	}
}

#endif // CS_MANAGERS_HPP
