#version 460

layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0, rgba32f) uniform image2D image;
// layout(binding = 0, std430) buffer _block_name {
// };

void main() {
	ivec2 size = imageSize(image);
	if(gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y)
		return;
	ivec2 iC = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = iC / vec2(imageSize(image) - 1);
	imageStore(image, iC, vec4(uv, 1, 1));
}
