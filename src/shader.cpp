#include "shader.hpp"

enum class shader_type : GLenum {
	compute = GL_COMPUTE_SHADER,
	vertex = GL_VERTEX_SHADER,
	fragment = GL_FRAGMENT_SHADER,
};

[[nodiscard]] inline GLuint make_shader(shader_type type, char const * source, char * error_buffer, GLsizei buffer_size, GLsizei * length) noexcept {
	auto handle = glCreateShader(static_cast<GLenum>(type));
	glShaderSource(handle, 1, &source, nullptr);
	glCompileShader(handle);
#ifdef _DEBUG
	GLint success;
	glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
	if(success)
		return handle;
	glGetShaderInfoLog(handle, buffer_size, length, error_buffer);
	glDeleteShader(handle);
	return 0;
#else // _DEBUG
	return handle;
#endif // _DEBUG
}

shader_program::shader_program(char const * compute_source, char * error_buffer, GLsizei buffer_size, GLsizei * length) noexcept {
	auto shader = make_shader(shader_type::compute, compute_source, error_buffer, buffer_size, length);
	if(!shader)
		return;
	auto handle = glCreateProgram();
	glAttachShader(handle, shader);
	glLinkProgram(handle);
	glDetachShader(handle, shader);
	glDeleteShader(shader);
#ifdef _DEBUG
	GLint success;
	glGetProgramiv(handle, GL_LINK_STATUS, &success);
	if(success) {
		_handle = handle;
		return;
	}
	glGetProgramInfoLog(handle, buffer_size, length, error_buffer);
	glDeleteProgram(handle);
#else // _DEBUG
      _handle = handle;
#endif // _DEBUG
}

shader_program::shader_program(char const * vertex_source, char const * fragment_source, char * error_buffer, GLsizei buffer_size, GLsizei * length) noexcept {
	auto vertex_shader = make_shader(shader_type::vertex, vertex_source, error_buffer, buffer_size, length);
	if(!vertex_shader)
		return;
	auto fragment_shader = make_shader(shader_type::fragment, fragment_source, error_buffer, buffer_size, length);
	if(!fragment_shader) {
		glDeleteShader(vertex_shader);
		return;
	}
	auto handle = glCreateProgram();
	glAttachShader(handle, vertex_shader);
	glAttachShader(handle, fragment_shader);
	glLinkProgram(handle);
	glDetachShader(handle, vertex_shader);
	glDetachShader(handle, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
#ifdef _DEBUG
	GLint success;
	glGetProgramiv(handle, GL_LINK_STATUS, &success);
	if(success) {
		_handle = handle;
		return;
	}
	glGetProgramInfoLog(handle, buffer_size, length, error_buffer);
	glDeleteProgram(handle);
#else // _DEBUG
      _handle = handle;
#endif // _DEBUG
}

shader_program::~shader_program() {
	glDeleteProgram(_handle);
}

void shader_program::use_program() const noexcept {
	glUseProgram(_handle);
}
