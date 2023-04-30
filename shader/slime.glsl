#version 460

layout(local_size_x = 1) in;

struct Agent {
	vec2 pos;
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
layout(location = 4) uniform Species _species[4];
layout(binding = 0, rgba32f) uniform image2D image;
layout(binding = 0, std430) buffer _block_name {
	Agent agents[];
};

ivec4 _speciesMask(uint species) {
	switch(species) {
	case 0: return ivec4(1, 0, 0, 0);
	case 1: return ivec4(0, 1, 0, 0);
	case 2: return ivec4(0, 0, 1, 0);
	case 3: return ivec4(0, 0, 0, 1);
	}
}

const uint index = gl_GlobalInvocationID.x;
const ivec2 size = imageSize(image);
const ivec4 speciesMask = _speciesMask(agents[index].species);
const ivec4 speciesMult = speciesMask * 2 - 1;
const Species species = _species[agents[index].species];

// Bob Jenkins
void hash(inout uint state) {
	state += (state << 10);
	state ^= (state >> 6);
	state += (state << 3);
	state ^= (state >> 11);
	state += (state << 15);
}

uint randomState() {
	uint idx = index;
	uint micros = uint(time * 1000000);
	hash(idx);
	hash(micros);
	return idx + micros;
}

float random01(inout uint state) {
	const float MAX_UINT = 4294967295.0; // (2^16-1)
	float random = state / MAX_UINT;
	hash(state);
	return random;
}

float sense(vec2 pos, float angle) {
	vec2 dir = {cos(angle), sin(angle)};
	ivec2 sensorPos = ivec2(pos + dir * species.sensorDistance);
	float sensed = 0;
	for(int x = -1; x <= 1; ++x)
		for(int y = -1; y <= 1; ++y)
			sensed += dot(imageLoad(image, sensorPos + ivec2(x, y)), speciesMult);
	return sensed;
}

void move(inout Agent agent, inout uint state) {
	vec2 pos = agent.pos;
	float angle = agent.angleRadians;
	float spacing = species.sensorSpacingRadians;
	float sensed = sense(pos, angle);
	float sensedLeft = sense(pos, angle + spacing);
	float sensedRight = sense(pos, angle - spacing);
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
	if(index >= numAgents)
		return;
	Agent agent = agents[index];
	uint state = randomState();
	move(agent, state);
	if(agent.pos.x < 0 || agent.pos.y < 0 || agent.pos.x > size.x || agent.pos.y > size.y) {
		agent.pos = clamp(agent.pos, vec2(0), size);
		agent.angleRadians = random01(state) * 2 * PI; // new random angle
	}
	agents[index] = agent;
	ivec2 imageCoords = ivec2(agent.pos);
	vec4 trail = overlapping == 1 ? max(speciesMask + imageLoad(image, imageCoords), 1) : speciesMask;
	imageStore(image, imageCoords, trail);
}
