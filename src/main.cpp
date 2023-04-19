#include <atomic>
#include <iostream>
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "managers.hpp"
#include "shader.hpp"
#include "shadersrc.hpp"

#define ERROR_FROM_MAIN(msg) { std::cerr << msg; return -1; }

struct glfw_context {
	~glfw_context() { glfwTerminate(); }
};

struct imgui_context {
	imgui_context(GLFWwindow * win) noexcept {
		ImGui::CreateContext();
		ImGui_ImplGlfw_InitForOpenGL(win, true);
		ImGui_ImplOpenGL3_Init();
	}

	~imgui_context() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
};

static void imgui_new_frame() noexcept {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

static void imgui_render() noexcept {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

struct size { int width, height; };
static std::atomic<size> framebuffer;
static slime::agent agents[1'000'000];

int main() {
	if(!glfwInit())
		ERROR_FROM_MAIN("glfwInit() failed\n");
	glfw_context _glfw;
	auto window = glfwCreateWindow(1920, 1080, "Compute Shaders", nullptr, nullptr);
	if(!window)
		ERROR_FROM_MAIN("glfwCreateWindow() failed\n");
	// glfwSwapInterval(0);
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebuffer.store({width, height}, std::memory_order_relaxed);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *, int width, int height) {
		framebuffer.store({width, height}, std::memory_order_relaxed);
		glViewport(0, 0, width, height);
	});
	glfwMakeContextCurrent(window);
	if(!gladLoadGL())
		ERROR_FROM_MAIN("gladLoadGL() failed\n");
	shader_program render_program{VERTEX_GLSL, FRAGMENT_GLSL};
	imgui_context _imgui{window};
	ImGui::GetIO().IniFilename = nullptr;
	constexpr float vertices[]{
		-1.0f, -1.0f, // bottom left
		 1.0f, -1.0f, // bottom right
		 1.0f,  1.0f, // top right
		-1.0f,  1.0f, // top left
	};
	constexpr char indices[]{
		0, 1, 2, // bottom right
		0, 3, 2, // top left
	};
	GLuint vao; glGenVertexArrays(1, &vao);
	GLuint buffers[3]; auto & [vbo, ebo, ssbo] = buffers; glGenBuffers(3, buffers);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glNamedBufferData(vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glNamedBufferData(ebo, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	auto setup_function = slime::setup_circle;
	slime::init(agents, width, height, setup_function);
	glNamedBufferData(ssbo, sizeof(agents), agents, GL_DYNAMIC_COPY); // TODO rethink usage
	bool open{};
	while(!glfwWindowShouldClose(window)) {
		auto sub_data = false, setup_needed = false, reset_needed = false;
		if(ImGui::IsKeyPressed(ImGuiKey_R, false)) {
			sub_data = reset_needed = true;
		} else if(ImGui::IsKeyPressed(ImGuiKey_C, false)) {
			setup_function = slime::setup_circle;
			sub_data = setup_needed = reset_needed = true;
		} else if(ImGui::IsKeyPressed(ImGuiKey_V, false)) {
			setup_function = slime::setup_uniform;
			sub_data = setup_needed = reset_needed = true;
		}
		auto [width, height] = framebuffer.load(std::memory_order_relaxed);
		if(slime::resize_and_setup_if_needed(width, height, setup_function)) {
			sub_data = true;
		} else {
			if(setup_needed)
				setup_function();
			if(reset_needed)
				slime::reset();
		}
		if(sub_data)
			glNamedBufferSubData(ssbo, 0, sizeof(agents), agents);
		slime::compute();
		render_program.use_program();
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
		imgui_new_frame();
		if(slime::draw_settings_window(open))
			glNamedBufferSubData(ssbo, 0, sizeof(agents), agents);
		if(open)
			imgui_render();
		else
			ImGui::EndFrame();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
