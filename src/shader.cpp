#include "shader.hpp"
#ifdef _DEBUG
#include <exception>
#include <iostream>
#endif

enum class shader_type : GLenum {
	compute = GL_COMPUTE_SHADER,
	vertex = GL_VERTEX_SHADER,
	fragment = GL_FRAGMENT_SHADER,
};

[[nodiscard]] static GLuint make_shader(shader_type type, char const * source) noexcept {
	auto shader = glCreateShader(static_cast<GLenum>(type));
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);
#ifdef _DEBUG
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success) {
		char buffer[1024];
		glGetShaderInfoLog(shader, 1024, nullptr, buffer);
		std::cerr << buffer;
		std::terminate();
	}
#endif // _DEBUG
	return shader;
}

static void link_program(GLuint program) noexcept {
	glLinkProgram(program);
#ifdef _DEBUG
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success) {
		char buffer[1024];
		glGetProgramInfoLog(program, 1024, nullptr, buffer);
		std::cerr << buffer;
		std::terminate();
	}
#endif // _DEBUG
}

GLuint make_program(char const * vertex_source, char const * fragment_source) noexcept {
	auto vertex_shader = make_shader(shader_type::vertex, vertex_source);
	auto fragment_shader = make_shader(shader_type::fragment, fragment_source);
	auto program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	link_program(program);
	glDetachShader(program, vertex_shader);
	glDetachShader(program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return program;
}

GLuint make_program(char const * compute_source) noexcept {
	auto shader = make_shader(shader_type::compute, compute_source);
	auto program = glCreateProgram();
	glAttachShader(program, shader);
	link_program(program);
	glDetachShader(program, shader);
	glDeleteShader(shader);
	return program;
}

shader_program::shader_program(char const * vertex_source, char const * fragment_source) noexcept
	: _handle{make_program(vertex_source, fragment_source)} {}

shader_program::shader_program(char const * compute_source) noexcept
	: _handle{make_program(compute_source)} {}

shader_program::~shader_program() {
	glDeleteProgram(_handle);
}

void shader_program::use_program() const noexcept {
	glUseProgram(_handle);
}
