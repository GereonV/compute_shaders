#version 460

layout(local_size_x = 32, local_size_y = 32) in;

layout(location = 0) uniform float deltaTime; // must be set
layout(location = 1) uniform float decayRate = 0.1; // must be non-negative
layout(location = 2) uniform float diffuseRate = 3; // must be non-negative
layout(location = 3) uniform vec3 species_colors[4];
layout(binding = 0, rgba32f) uniform image2D image;
layout(binding = 1, rgba32f) uniform image2D coloredImage;

vec4 speciesMask(uint species) {
	switch(species) {
	case 0: return vec4(1, 0, 0, 0);
	case 1: return vec4(0, 1, 0, 0);
	case 2: return vec4(0, 0, 1, 0);
	case 3: return vec4(0, 0, 0, 1);
	}
}

void main() {
	ivec2 size = imageSize(image);
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if(pos.x >= size.x || pos.y >= size.y)
		return;
	vec4 original = imageLoad(image, pos);
	vec4 sum = vec4(0);
	for(int x = -1; x <= 1; ++x)
		for(int y = -1; y <= 1; ++y)
			sum += imageLoad(image, clamp(ivec2(x, y) + pos, ivec2(0), size));
	vec4 blurred = sum / 9;
	vec4 diffused = mix(original, blurred, diffuseRate * float(deltaTime));
	vec4 decayed = max(diffused - decayRate * float(deltaTime), 0);
	imageStore(image, pos, decayed);
	vec3 colored = vec3(0);
	for(uint species = 0; species < 4; ++species)
		colored += dot(speciesMask(species), decayed) * species_colors[species];
	imageStore(coloredImage, pos, vec4(colored, 1));
}
