#version 460

layout(local_size_x = 1) in;

struct Agent {
	vec2 pos; // ∈ [0, 1]
	float angleRadians;
	uint species; // ∈ [0, 3]
};

struct Species {
	float moveSpeed;
	float turnRadiansPerSecond;
	float sensorSpacingRadians;
	float sensorDistance;
};

#define PI 3.1415926535897932384626433832795
layout(location = 0) uniform float time; // as seed
layout(location = 1) uniform float deltaTime;
layout(location = 2) uniform Species species[4];
layout(binding = 0, rgba32f) uniform image2D image;
layout(binding = 0, std430) buffer _block_name {
	Agent agents[];
};

// Bob Jenkins
void hash(inout uint state) {
	state += (state << 10);
	state ^= (state >> 6);
	state += (state << 3);
	state ^= (state >> 11);
	state += (state << 15);
}

uint randomState(uint index) {
	uint micros = uint(time * 1000000);
	hash(index);
	hash(micros);
	return index + micros;
}

float random01(inout uint state) {
	const float MAX_UINT = 4294967295.0; // (2^16-1)
	float random = state / MAX_UINT;
	hash(state);
	return random;
}

vec4 speciesMask(uint species) {
	switch(species) {
	case 0: return vec4(1, 0, 0, 0);
	case 1: return vec4(0, 1, 0, 0);
	case 2: return vec4(0, 0, 1, 0);
	case 3: return vec4(0, 0, 0, 1);
	}
}

void main() {
	uint index = gl_GlobalInvocationID.x;
	uint numAgents = agents.length();
	if(index >= numAgents)
		return;
	Agent agent = agents[index];
	Species species = species[agent.species];
	vec2 dir = {cos(agent.angleRadians), sin(agent.angleRadians)};
	float moveDistance = species.moveSpeed * deltaTime;
	vec2 newPos = agent.pos + dir * moveDistance;
	if(newPos.x < 0 || newPos.y < 0 || newPos.x > 1 || newPos.y > 1) {
		newPos = clamp(newPos, 0, 1);
		uint state = randomState(index);
		agents[index].angleRadians = random01(state) * 2 * PI; // new random angle
	}
	agents[index].pos = newPos;
	ivec2 size = imageSize(image);
	ivec2 imageCoords = min(ivec2(newPos * size), size - 1);
	imageStore(image, imageCoords, speciesMask(agent.species));
}
