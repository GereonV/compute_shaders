#include <glad/glad.h>
#include <imgui.h>
#include "app.hpp"
#include "managers.hpp"

static slime::agent agents[1'000'000];

int main() {
	init();
	GLuint ssbo;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	auto [width, height] = framebuffer_size();
	auto setup_function = slime::setup_circle;
	slime::init(agents, width, height, setup_function);
	glNamedBufferData(ssbo, sizeof(agents), agents, GL_DYNAMIC_COPY); // TODO rethink usage
	bool open{};
	while(new_frame()) {
		if(slime::draw_settings_window(open))
			glNamedBufferSubData(ssbo, 0, sizeof(agents), agents);
		auto sub_data = false, reset_needed = false;
		if(ImGui::IsKeyPressed(ImGuiKey_R, false)) {
			sub_data = reset_needed = true;
		} else if(ImGui::IsKeyPressed(ImGuiKey_C, false)) {
			setup_function = slime::setup_circle;
			sub_data = reset_needed = true;
		} else if(ImGui::IsKeyPressed(ImGuiKey_V, false)) {
			setup_function = slime::setup_uniform;
			sub_data = reset_needed = true;
		}
		auto [width, height] = framebuffer_size();
		if(slime::resize_and_setup_if_needed(width, height, setup_function)) {
			sub_data = true;
		} else if(reset_needed) {
			setup_function();
			slime::reset();
		}
		if(sub_data)
			glNamedBufferSubData(ssbo, 0, sizeof(agents), agents);
		slime::compute();
		render();
	}
}
