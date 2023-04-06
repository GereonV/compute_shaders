#version 460

layout(local_size_x = 1) in;

layout(binding = 0, rgba32f) uniform image2D uImage;

// layout(std430, binding = 0) buffer block_name {
// 	vec4 pixels[]; // vec3 are aligned as vec4 anyway
// };

void main() {
	ivec2 size = imageSize(uImage);
	if(gl_WorkGroupID.x >= size.x || gl_WorkGroupID.y >= size.y)
		return;
	vec2 uvCoords = vec2(gl_WorkGroupID) / (gl_NumWorkGroups.xy - 1);
	ivec2 imageCoords = ivec2(gl_WorkGroupID.xy);
	imageStore(uImage, imageCoords, vec4(uvCoords, 0, 1));

	// uint pixelIndex = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
	// pixels[pixelIndex] = vec4(uvCoords, 0, 1);
}
