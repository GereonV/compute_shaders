#include "managers.hpp"

texture_manager::~texture_manager() {
	glDeleteTextures(1, &_texture);
}

void texture_manager::reset(GLsizei width, GLsizei height) noexcept {
	glDeleteTextures(1, &_texture);
	glCreateTextures(GL_TEXTURE_2D, 1, &_texture);
	glTextureStorage2D(_texture, 1, GL_RGBA32F, width, height);
}

void texture_manager::bind_to_texture_unit(GLuint unit) const noexcept {
	glBindTextureUnit(unit, _texture);
}

void texture_manager::bind_to_image_unit(GLuint unit, shader_image_access access) const noexcept {
	glBindImageTexture(unit, _texture, 0, false, 0, static_cast<GLenum>(access), GL_RGBA32F);
}
