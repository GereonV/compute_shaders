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
layout(location = 2) uniform uint numAgents;
layout(location = 3) uniform Species species[4];
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

ivec4 speciesMask(uint species) {
	switch(species) {
	case 0: return ivec4(1, 0, 0, 0);
	case 1: return ivec4(0, 1, 0, 0);
	case 2: return ivec4(0, 0, 1, 0);
	case 3: return ivec4(0, 0, 0, 1);
	}
}

float sense(vec2 pos, ivec2 size, ivec4 speciesMask) {
	ivec2 imageCoords = ivec2(pos * size);
	float sensed = 0;
	for(int x = -1; x <= 1; ++x)
		for(int y = -1; y <= 1; ++y)
			sensed += dot(imageLoad(image, imageCoords + ivec2(x, y)), speciesMask);
	return sensed;
}

void move(inout Agent agent, Species species, ivec2 size, ivec4 speciesMask) {
	float angle = agent.angleRadians;
	float spacing = species.sensorSpacingRadians;
	vec2 dir = {cos(angle), sin(angle)};
	vec2 dirLeft = {cos(angle + spacing), sin(angle + spacing)};
	vec2 dirRight = {cos(angle - spacing), sin(angle - spacing)};
	vec2 pos = agent.pos;
	float distance = species.sensorDistance;
	speciesMask = speciesMask * 2 - 1;
	float sensed = sense(pos + dir * distance, size, speciesMask);
	float sensedLeft = sense(pos + dirLeft * distance, size, speciesMask);
	float sensedRight = sense(pos + dirRight * distance, size, speciesMask);
	float turnAmount = species.turnRadiansPerSecond * deltaTime;
	if(sensedLeft > sensed && sensedLeft > sensedRight) {
		agent.angleRadians += turnAmount;
		dir = dirLeft;
	} else if(sensedRight > sensed && sensedRight > sensedLeft) {
		agent.angleRadians -= turnAmount;
		dir = dirRight;
	}
	float moveDistance = species.moveSpeed * deltaTime;
	agent.pos += dir * moveDistance;
}

void main() {
	uint index = gl_GlobalInvocationID.x;
	if(index >= numAgents)
		return;
	Agent agent = agents[index];
	Species species = species[agent.species];
	ivec2 size = imageSize(image);
	ivec4 speciesMask = speciesMask(agent.species);
	move(agent, species, size, speciesMask);
	if(agent.pos.x < 0 || agent.pos.y < 0 || agent.pos.x > 1 || agent.pos.y > 1) {
		agent.pos = clamp(agent.pos, 0, 1);
		uint state = randomState(index);
		agent.angleRadians = random01(state) * 2 * PI; // new random angle
	}
	agents[index] = agent;
	imageStore(image, ivec2(agent.pos * size), speciesMask);
}
