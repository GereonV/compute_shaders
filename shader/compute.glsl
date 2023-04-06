#version 460

layout(local_size_x = 1) in;

layout(binding = 0, rgba32f) uniform image2D uImage;

void main() {
	ivec2 size = imageSize(uImage);
	vec2 uvCoords = vec2(gl_WorkGroupID) / (gl_NumWorkGroups.xy - 1);
	ivec2 imageCoords = ivec2(gl_WorkGroupID.xy);
	imageStore(uImage, imageCoords, vec4(uvCoords, 0, 1));
}
