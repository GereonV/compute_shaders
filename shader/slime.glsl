#version 460

layout(local_size_x = 1) in;

struct Agent {
	vec2 pos;
	float angle;
};

#define PI 3.1415926535897932384626433832795

layout(location = 0) uniform double time; // as seed
layout(location = 1) uniform double deltaTime; // must be set
layout(location = 2) uniform float moveSpeed = 0.15;
layout(location = 3) uniform float turnSpeed = PI;
layout(location = 4) uniform float sensorAngle = PI / 6;
layout(location = 5) uniform float sensorDistance = 35;
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
	float random = state / 4294967295.0;
	hash(state);
	return random;
}

void main() {
	uint index = gl_GlobalInvocationID.x;
	uint numAgents = agents.length();
	if(index >= numAgents)
		return;
	Agent agent = agents[index];
	vec2 dir = {cos(agent.angle), sin(agent.angle)};
	float moveDistance = moveSpeed * float(deltaTime);
	vec2 newPos = agent.pos + dir * moveDistance;
	uint state = randomState(index);
	if(newPos.x < 0 || newPos.y < 0 || newPos.x > 1 || newPos.y > 1) {
		newPos = clamp(newPos, 0, 1);
		agents[index].angle = random01(state) * 2 * PI;
	} else {
		float turn = turnSpeed * random01(state) * float(deltaTime);
		// agents[index].angle += turn;
	}
	agents[index].pos = newPos;
	ivec2 size = imageSize(image);
	ivec2 imageCoords = ivec2(newPos * (size + 0.99999999));
	imageStore(image, imageCoords, vec4(1));
	ivec2 center = imageSize(image) / 2;
}
