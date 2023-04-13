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

inline void imgui_new_frame() noexcept {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

inline void imgui_render() noexcept {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// inline?
static std::atomic_flag framebuffer_changed;
static int width, height;
static slime::agent agents[10000];

int main() {
	if(!glfwInit())
		ERROR_FROM_MAIN("glfwInit() failed\n");
	glfw_context _glfw;
	auto window = glfwCreateWindow(1920, 1080, "Compute Shaders", nullptr, nullptr);
	if(!window)
		ERROR_FROM_MAIN("glfwCreateWindow() failed\n");
	// glfwSwapInterval(0);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *, int width, int height) {
		::width = width;
		::height = height;
		framebuffer_changed.test_and_set(std::memory_order_release);
		glViewport(0, 0, width, height);
	});
	glfwMakeContextCurrent(window);
	if(!gladLoadGL())
		ERROR_FROM_MAIN("gladLoadGL() failed\n");
	shader_program_builder builder;
	auto render_program = builder.build(VERTEX_GLSL, FRAGMENT_GLSL);
	if(builder.error())
		ERROR_FROM_MAIN(builder.error_message());
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
	std::mt19937 twister;
	std::uniform_real_distribution<float> dist01;
	std::uniform_real_distribution<float> angle_dist{0, 2 * 3.1415926535f};
	std::uniform_int_distribution<int> species_dist{0, slime::manager::species_count - 1};
	for(auto & agent : agents) {
		agent.x = dist01(twister);
		agent.y = dist01(twister);
		agent.angle_radians = angle_dist(twister);
		agent.species = species_dist(twister);
	}
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
	glNamedBufferData(ssbo, sizeof(agents), agents, GL_DYNAMIC_COPY); // TODO rethink usage
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	auto manager = slime::make_manager();
	set_colors_to_default(manager);
	bool show_settings{};
	while(!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		if(framebuffer_changed.test(std::memory_order_acquire)) {
			framebuffer_changed.clear(std::memory_order_relaxed);
			if(manager.resize(width, height))
				glNamedBufferSubData(ssbo, 0, sizeof(agents), agents);
		}
		manager.compute(sizeof(agents) / sizeof(*agents));
		render_program.use_program();
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
		imgui_new_frame();
		if(ImGui::IsKeyPressed(ImGuiKey_S, false))
			show_settings = true;
		if(show_settings) {
			if(!manager.draw_settings_window())
				show_settings = false;
			imgui_render();
		} else {
			ImGui::EndFrame();
		}
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
