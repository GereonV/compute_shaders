#include "managers.hpp"
#include <glad/glad.h>
#include "app.hpp"
#include "shader.hpp"
#include "shadersrc.hpp"

static GLsizei width, height;

static GLuint ssbo;
static GLuint texture;
static GLuint program;

static void create_texture(GLsizei width, GLsizei height) noexcept {
	::width = width;
	::height = height;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureStorage2D(texture, 1, GL_RGBA32F, width, height);
	glClearTexImage(texture, 0, GL_RGBA, GL_FLOAT, nullptr);
	glBindImageTexture(0, texture, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);
	glBindTextureUnit(0, texture);
}

void rt::init() noexcept {
	auto [width, height] = framebuffer_size();
	glGenBuffers(1, &ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	create_texture(width, height);
	program = make_program(RT_GLSL);
}

void rt::shutdown() noexcept {
	glDeleteBuffers(1, &ssbo);
	glDeleteTextures(1, &texture);
	glDeleteProgram(program);
}

void rt::compute() noexcept {
	auto [width, height] = framebuffer_size();
	if(::width != width || ::height != height) {
		glDeleteTextures(1, &texture);
		create_texture(width, height);
	}
	glUseProgram(program);
	glDispatchCompute(width / 8 + 1, height / 8 + 1, 1);
}
