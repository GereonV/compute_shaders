#ifndef CS_SHADER_HPP
#define CS_SHADER_HPP

#include <glad/glad.h>

[[nodiscard]] GLuint make_program(char const * vertex_source, char const * fragment_source) noexcept;
[[nodiscard]] GLuint make_program(char const * compute_source) noexcept;

class shader_program {
public:
	shader_program(char const * vertex_source, char const * fragment_source) noexcept;
	shader_program(char const * compute_source) noexcept;
	shader_program(shader_program const &) = delete;
	~shader_program();
	void use_program() const noexcept;
private:
	GLuint _handle;
};

#endif // CS_SHADER_HPP
