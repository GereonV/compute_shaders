#version 460

in vec2 uvCoords;
layout(binding = 0) uniform sampler2D uSampler;
out vec4 outColor;

void main() {
	outColor = texture(uSampler, uvCoords);
}
