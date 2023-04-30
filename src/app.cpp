#include "app.hpp"
#include "shader.hpp"
#include <atomic>
#include <exception>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "shadersrc.hpp"

static GLFWwindow * window;
static std::atomic<size> framebuffer;
static GLuint program;

void init() noexcept {
	if(!glfwInit())
		terminate("glfwInit() failed\n");
	window = glfwCreateWindow(1920, 1080, "Compute Shaders", nullptr, nullptr);
	if(!window)
		terminate("glfwCreateWindow() failed\n");
	glfwMakeContextCurrent(window);
	if(!gladLoadGL())
		terminate("gladLoadGL() failed\n");
	program = make_program(VERTEX_GLSL, FRAGMENT_GLSL);
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer.store({width, height}, std::memory_order_relaxed);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *, int width, int height) {
		framebuffer.store({width, height}, std::memory_order_relaxed);
		glViewport(0, 0, width, height);
	});
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	ImGui::GetIO().IniFilename = nullptr;
	constexpr float vertices[]{
		 1.0f, -1.0f, // bottom right
		-1.0f, -1.0f, // bottom left
		 1.0f,  1.0f, // top right
		-1.0f,  1.0f, // top left
	};
	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glNamedBufferData(vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// TODO separate vertex attributes
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(0);
}

void deinit() noexcept {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	window = nullptr;
	glfwTerminate();
}

void terminate(char const * msg) noexcept {
	glfwTerminate();
	std::cerr << msg;
	std::terminate();
}

bool new_frame() noexcept {
	if(glfwWindowShouldClose(window))
		return false;
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	return true;
}

void render() noexcept {
	glUseProgram(program);
	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window);
	glfwPollEvents();
}

size framebuffer_size() noexcept {
	return framebuffer.load(std::memory_order_relaxed);
}
