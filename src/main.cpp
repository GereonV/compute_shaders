#include <iostream>
#include <mutex>
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "managers.hpp"
#include "shader.hpp"
#include "shadersrc.hpp"

#define ERROR_FROM_MAIN(msg) { std::cerr << msg; return -1; }

struct glfw_context {
	~glfw_context() { glfwTerminate(); }
};

static struct {
	float x, y, angle, _;
} agents[1000];

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
	// glfwSwapInterval(0);
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
	auto slime_program = builder.build(SLIME_GLSL);
	if(builder.error())
		ERROR_FROM_MAIN(builder.error_message());
	auto postprocess_program = builder.build(POSTPROCESS_GLSL);
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
	std::mt19937 twister;
	std::uniform_real_distribution<float> dist01;
	std::uniform_real_distribution<float> angle_dist{0, 2 * 3.1415926535f};
	for(auto & agent : agents) {
		agent.x = dist01(twister);
		agent.y = dist01(twister);
		agent.angle = angle_dist(twister);
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
	auto [manager, width, height] = framebuffer.create_texture_manager();
	manager.clear();
	manager.bind_to_image_unit(0, shader_image_access::read_write);
	manager.bind_to_texture_unit(0);
	double last_time{};
	while(!glfwWindowShouldClose(window)) {
		auto time = glfwGetTime();
		auto delta_time = time - last_time;
		glClear(GL_COLOR_BUFFER_BIT);
		framebuffer.reset_and_call_if_changed(
			[&](GLsizei new_width, GLsizei new_height) {
				width = new_width;
				height = new_height;
				manager.reset(width, height);
				manager.clear();
				manager.bind_to_image_unit(0, shader_image_access::read_write);
				manager.bind_to_texture_unit(0);
				glNamedBufferSubData(ssbo, 0, sizeof(agents), agents);
			}
		);
		slime_program.use_program();
		glUniform1d(0, time);
		glUniform1d(1, delta_time);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDispatchCompute(sizeof(agents) / sizeof(*agents), 1, 1);
		postprocess_program.use_program();
		glUniform1d(0, delta_time);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glDispatchCompute(width / 32 + 1, height / 32 + 1, 1);
		render_program.use_program();
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT); // compute result
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
		glfwSwapBuffers(window);
		glfwPollEvents();
		last_time = time;
	}
}
