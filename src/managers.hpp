#ifndef CS_MANAGERS_HPP
#define CS_MANAGERS_HPP

#include <glad/glad.h>

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

#endif // CS_MANAGERS_HPP
