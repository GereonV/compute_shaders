#ifndef CS_SHADER_HPP
#define CS_SHADER_HPP

#include <string_view>
#include <glad/glad.h>

class shader_program {
template<GLsizei N> friend class shader_program_builder;
private:
	shader_program(char const * compute_source, char * error_buffer, GLsizei buffer_size, GLsizei * length) noexcept;
	shader_program(char const * vertex_source, char const * fragment_source, char * error_buffer, GLsizei buffer_size, GLsizei * length) noexcept;
public:
	shader_program(shader_program const &) = delete;
	~shader_program();
	void use_program() const noexcept;
protected:
	GLuint _handle{};
};

template<GLsizei N = 1024>
class shader_program_builder {
public:
	template<typename... T>
	[[nodiscard]] constexpr shader_program build(T &&... args) noexcept {
		_error_length = 0;
		return {static_cast<T &&>(args)..., _error_buffer, N, &_error_length};
	}

	[[nodiscard]] constexpr bool error() const noexcept {
		return _error_length;
	}

	[[nodiscard]] constexpr std::string_view error_message() const noexcept {
		return {_error_buffer, static_cast<std::size_t>(_error_length)};
	}
private:
	char _error_buffer[N];
	GLsizei _error_length;
};

#endif // CS_SHADER_HPP
