#ifndef CS_MANAGERS_HPP
#define CS_MANAGERS_HPP

#include <numbers>
#include <glad/glad.h>

namespace slime {
	void init() noexcept;
	void shutdown() noexcept;
	void compute() noexcept;
}

#endif // CS_MANAGERS_HPP
