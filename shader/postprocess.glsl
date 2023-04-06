#version 460

layout(local_size_x = 32, local_size_y = 32) in;

layout(location = 0) uniform float uDeltaTime; // must be set
layout(location = 1) uniform float uDecayRate = 1; // must be non-negative
layout(location = 2) uniform float uDiffuseRate = 1; // must be non-negative
layout(binding = 0, rgba32f) uniform image2D uImage;

void main() {
	ivec2 size = imageSize(uImage);
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if(pos.x >= size.x || pos.y >= size.y)
		return;
	vec4 original = imageLoad(uImage, pos);
	vec4 sum = vec4(0);
	for(int x = -1; x <= 1; ++x)
		for(int y = -1; y <= 1; ++y)
			sum += imageLoad(uImage, clamp(ivec2(x, y) + pos, ivec2(0), size));
	vec4 blurred = sum / 9;
	vec4 diffused = mix(original, blurred, uDiffuseRate * uDeltaTime);
	vec4 decayed = max(diffused - uDecayRate * uDeltaTime, 0);
	imageStore(uImage, pos, decayed);
}
