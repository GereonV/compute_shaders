#version 460

in vec2 uvCoords;
out vec4 outColor;

void main() {
	outColor = vec4(uvCoords.xy, 0, 1);
}
