#version 460

layout(location = 0) in vec2 inPos;
out vec2 uvCoords;

void main() {
	gl_Position = vec4(inPos, 0, 1);
	uvCoords = (inPos + 1) / 2;
}
