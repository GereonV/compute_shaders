#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "managers.hpp"
#include "shader.hpp"
#include "shadersrc.hpp"

#define ERROR_FROM_MAIN(msg) { std::cerr << msg; return -1; }

struct glfw_context {
	~glfw_context() { glfwTerminate(); }
};

static bool framebuffer_changed{};
static GLsizei width, height;

int main() {
	if(!glfwInit())
		ERROR_FROM_MAIN("glfwInit() failed\n");
	glfw_context _glfw;
	auto window = glfwCreateWindow(1920, 1080, "Compute Shaders", nullptr, nullptr);
	if(!window)
		ERROR_FROM_MAIN("glfwCreateWindow() failed\n");
	glfwGetFramebufferSize(window, &width, &height);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *, int width, int height) {
		framebuffer_changed = true;
		::width = width;
		::height = height;
		glViewport(0, 0, width, height);
	});
	if(!gladLoadGL())
		ERROR_FROM_MAIN("gladLoadGL() failed\n");
	shader_program_builder builder;
	auto render_program = builder.build(VERTEX_GLSL, FRAGMENT_GLSL);
	if(builder.error())
		ERROR_FROM_MAIN(builder.error_message());
	auto compute_program = builder.build(COMPUTE_GLSL);
	if(builder.error())
		ERROR_FROM_MAIN(builder.error_message());
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
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	// glNamedBufferData(ssbo, sizeof(float) * 4 * 1920 * 1080, nullptr, GL_DYNAMIC_COPY);
	// float * shader_storage = new float[4 * 1920 * 1080];
	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	texture_manager manager{width, height};
	while(!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		if(framebuffer_changed) {
			framebuffer_changed = false;
			manager.reset(width, height);
		}
		compute_program.use_program();
		manager.bind_to_image_unit(0, shader_image_access::write_only);
		glDispatchCompute(width, height, 1);
		// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		// glGetNamedBufferSubData(ssbo, 0, sizeof(float) * 4 * 1920 * 1080, shader_storage);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		render_program.use_program();
		manager.bind_to_texture_unit(0);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
