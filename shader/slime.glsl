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
layout(location = 3) uniform uint overlapping;
layout(location = 4) uniform Species species[4];
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

float sense(vec2 pos, float angle, float distance, ivec2 size, ivec4 speciesMult) {
	vec2 dir = {cos(angle), sin(angle)};
	vec2 sensorPos = pos + dir * distance;
	ivec2 imageCoords = ivec2(sensorPos * size);
	float sensed = 0;
	for(int x = -1; x <= 1; ++x)
		for(int y = -1; y <= 1; ++y)
			sensed += dot(imageLoad(image, imageCoords + ivec2(x, y)), speciesMult);
	return sensed;
}

void move(inout Agent agent, Species species, ivec2 size, ivec4 speciesMask, inout uint state) {
	vec2 pos = agent.pos;
	float angle = agent.angleRadians;
	float spacing = species.sensorSpacingRadians;
	float distance = species.sensorDistance;
	ivec4 speciesMult = speciesMask * 2 - 1;
	float sensed = sense(pos, angle, distance, size, speciesMult);
	float sensedLeft = sense(pos, angle + spacing, distance, size, speciesMult);
	float sensedRight = sense(pos, angle - spacing, distance, size, speciesMult);
	float turnAmount = species.turnRadiansPerSecond * deltaTime;
	if(sensedLeft > sensed && sensedLeft > sensedRight)
		agent.angleRadians += turnAmount;
	else if(sensedRight > sensed && sensedRight > sensedLeft)
		agent.angleRadians -= turnAmount;
	agent.angleRadians += (2 * random01(state) - 1) * PI / 2 * deltaTime;
	vec2 dir = {cos(agent.angleRadians), sin(agent.angleRadians)};
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
	uint state = randomState(index);
	move(agent, species, size, speciesMask, state);
	if(agent.pos.x < 0 || agent.pos.y < 0 || agent.pos.x > 1 || agent.pos.y > 1) {
		agent.pos = clamp(agent.pos, 0, 1);
		agent.angleRadians = random01(state) * 2 * PI; // new random angle
	}
	agents[index] = agent;
	ivec2 imageCoords = ivec2(agent.pos * size);
	vec4 prev = overlapping == 1 ? imageLoad(image, imageCoords) : vec4(0);
	imageStore(image, imageCoords, speciesMask + prev);
}
