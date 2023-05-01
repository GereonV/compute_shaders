#include "managers.hpp"
#include <glad/glad.h>
#include <imgui.h>
#include "app.hpp"
#include "shader.hpp"
#include "shadersrc.hpp"

struct sphere {
	float x, y, z;
	float r;
};

static float fov;
static sphere s;

static bool menu_open;

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

static void imgui() noexcept {
	if(ImGui::IsKeyPressed(ImGuiKey_S, false))
		menu_open = !menu_open;
	if(!menu_open)
		return;
	bool changed{};
	if(ImGui::Begin("Settings", &menu_open)) {
		ImGui::SliderFloat("FOV", &fov, 0.5f, 2.0f, nullptr, ImGuiSliderFlags_AlwaysClamp);
		if(ImGui::DragFloat3("Position", &s.x, 0.01f)) changed = true;
		if(ImGui::DragFloat("Radius", &s.r, 0.01f, 0.0f, FLT_MAX, nullptr, ImGuiSliderFlags_AlwaysClamp)) changed = true;
	}
	if(changed)
		glNamedBufferSubData(ssbo, 0, sizeof(sphere), &s);
	ImGui::End();
}

void rt::init() noexcept {
	fov = 1.0f;
	s = {0.0f, 0.0f, -1.0f, 0.25f};
	auto [width, height] = framebuffer_size();
	glGenBuffers(1, &ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glNamedBufferData(ssbo, sizeof(sphere), &s, GL_STATIC_DRAW);
	create_texture(width, height);
	program = make_program(RT_GLSL);
}

void rt::shutdown() noexcept {
	glDeleteBuffers(1, &ssbo);
	glDeleteTextures(1, &texture);
	glDeleteProgram(program);
}

void rt::compute() noexcept {
	imgui();
	auto [width, height] = framebuffer_size();
	if(::width != width || ::height != height) {
		glDeleteTextures(1, &texture);
		create_texture(width, height);
	}
	glUseProgram(program);
	glUniform1f(0, fov);
	glDispatchCompute(width / 8 + 1, height / 8 + 1, 1);
}
