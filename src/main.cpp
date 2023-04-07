#include <iostream>
#include <mutex>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "managers.hpp"
#include "shader.hpp"
#include "shadersrc.hpp"

#define ERROR_FROM_MAIN(msg) { std::cerr << msg; return -1; }

struct glfw_context {
	~glfw_context() { glfwTerminate(); }
};

static class {
public:
	void init_unsynchronized(GLFWwindow * window) noexcept {
		_changed = false;
		glfwGetFramebufferSize(window, &_width, &_height);
	}

	void change(GLsizei width, GLsizei height) noexcept {
		std::scoped_lock lock{_mutex};
		_changed = true;
		_width = width;
		_height = height;
	}

	template<typename F>
	void reset_and_call_if_changed(F && f) noexcept {
		std::unique_lock lock{_mutex};
		auto changed = _changed;
		if(!changed)
			return;
		_changed = false;
		lock.unlock();
		static_cast<F &&>(f)(_width, _height);
	}

	[[nodiscard]] auto create_texture_manager() const noexcept {
		struct result {
			texture_manager manager;
			GLsizei width, height;
		};
		_mutex.lock();
		auto width = _width;
		auto height = _height;
		_mutex.unlock();
		return result{{width, height}, width, height};
	}
private:
	mutable std::mutex _mutex;
	bool _changed;
	GLsizei _width, _height;
} framebuffer;

int main() {
	if(!glfwInit())
		ERROR_FROM_MAIN("glfwInit() failed\n");
	glfw_context _glfw;
	auto window = glfwCreateWindow(1920, 1080, "Compute Shaders", nullptr, nullptr);
	if(!window)
		ERROR_FROM_MAIN("glfwCreateWindow() failed\n");
	framebuffer.init_unsynchronized(window);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	glfwSetFramebufferSizeCallback(window, [](GLFWwindow *, int width, int height) {
		framebuffer.change(width, height);
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
	auto [manager, width, height] = framebuffer.create_texture_manager();
	auto compute = [&](GLsizei width, GLsizei height) {
		manager.bind_to_image_unit(0, shader_image_access::write_only);
		manager.bind_to_texture_unit(0);
		compute_program.use_program();
		glDispatchCompute(width, height, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		render_program.use_program();
	};
	compute(width, height);
	while(!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		framebuffer.reset_and_call_if_changed(
			[&](GLsizei width, GLsizei height) {
				manager.reset(width, height);
				compute(width, height);
			}
		);
		// glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		// glGetNamedBufferSubData(ssbo, 0, sizeof(float) * 4 * 1920 * 1080, shader_storage);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}
