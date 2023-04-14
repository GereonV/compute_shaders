#define COMPUTE_GLSL \
"#version 460\n" \
"\n" \
"layout(local_size_x = 1) in;\n" \
"\n" \
"layout(binding = 0, rgba32f) uniform image2D uImage;\n" \
"\n" \
"void main() {\n" \
"	ivec2 size = imageSize(uImage);\n" \
"	vec2 uvCoords = vec2(gl_WorkGroupID) / (gl_NumWorkGroups.xy - 1);\n" \
"	ivec2 imageCoords = ivec2(gl_WorkGroupID.xy);\n" \
"	imageStore(uImage, imageCoords, vec4(uvCoords, 0, 1));\n" \
"}\n" \
""
#define FRAGMENT_GLSL \
"#version 460\n" \
"\n" \
"in vec2 uvCoords;\n" \
"layout(binding = 0) uniform sampler2D uSampler;\n" \
"out vec4 outColor;\n" \
"\n" \
"void main() {\n" \
"	outColor = texture(uSampler, uvCoords);\n" \
"}\n" \
""
#define POSTPROCESS_GLSL \
"#version 460\n" \
"\n" \
"layout(local_size_x = 32, local_size_y = 32) in;\n" \
"\n" \
"layout(location = 0) uniform float deltaTime; // must be set\n" \
"layout(location = 1) uniform float decayRate = 0.1; // must be non-negative\n" \
"layout(location = 2) uniform float diffuseRate = 3; // must be non-negative\n" \
"layout(location = 3) uniform vec3 species_colors[4];\n" \
"layout(binding = 0, rgba32f) uniform image2D image;\n" \
"layout(binding = 1, rgba32f) uniform image2D coloredImage;\n" \
"\n" \
"vec4 speciesMask(uint species) {\n" \
"	switch(species) {\n" \
"	case 0: return vec4(1, 0, 0, 0);\n" \
"	case 1: return vec4(0, 1, 0, 0);\n" \
"	case 2: return vec4(0, 0, 1, 0);\n" \
"	case 3: return vec4(0, 0, 0, 1);\n" \
"	}\n" \
"}\n" \
"\n" \
"void main() {\n" \
"	ivec2 size = imageSize(image);\n" \
"	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n" \
"	if(pos.x >= size.x || pos.y >= size.y)\n" \
"		return;\n" \
"	vec4 original = imageLoad(image, pos);\n" \
"	vec4 sum = vec4(0);\n" \
"	for(int x = -1; x <= 1; ++x)\n" \
"		for(int y = -1; y <= 1; ++y)\n" \
"			sum += imageLoad(image, clamp(ivec2(x, y) + pos, ivec2(0), size));\n" \
"	vec4 blurred = sum / 9;\n" \
"	vec4 diffused = mix(original, blurred, diffuseRate * float(deltaTime));\n" \
"	vec4 decayed = max(diffused - decayRate * float(deltaTime), 0);\n" \
"	imageStore(image, pos, decayed);\n" \
"	vec3 colored = vec3(0);\n" \
"	for(uint species = 0; species < 4; ++species)\n" \
"		colored += dot(speciesMask(species), decayed) * species_colors[species];\n" \
"	imageStore(coloredImage, pos, vec4(colored, 1));\n" \
"}\n" \
""
#define SLIME_GLSL \
"#version 460\n" \
"\n" \
"layout(local_size_x = 1) in;\n" \
"\n" \
"struct Agent {\n" \
"	vec2 pos; // ∈ [0, 1]\n" \
"	float angleRadians;\n" \
"	uint species; // ∈ [0, 3]\n" \
"};\n" \
"\n" \
"struct Species {\n" \
"	float moveSpeed;\n" \
"	float turnRadiansPerSecond;\n" \
"	float sensorSpacingRadians;\n" \
"	float sensorDistance;\n" \
"};\n" \
"\n" \
"#define PI 3.1415926535897932384626433832795\n" \
"layout(location = 0) uniform float time; // as seed\n" \
"layout(location = 1) uniform float deltaTime;\n" \
"layout(location = 2) uniform uint numAgents;\n" \
"layout(location = 3) uniform Species species[4];\n" \
"layout(binding = 0, rgba32f) uniform image2D image;\n" \
"layout(binding = 0, std430) buffer _block_name {\n" \
"	Agent agents[];\n" \
"};\n" \
"\n" \
"// Bob Jenkins\n" \
"void hash(inout uint state) {\n" \
"	state += (state << 10);\n" \
"	state ^= (state >> 6);\n" \
"	state += (state << 3);\n" \
"	state ^= (state >> 11);\n" \
"	state += (state << 15);\n" \
"}\n" \
"\n" \
"uint randomState(uint index) {\n" \
"	uint micros = uint(time * 1000000);\n" \
"	hash(index);\n" \
"	hash(micros);\n" \
"	return index + micros;\n" \
"}\n" \
"\n" \
"float random01(inout uint state) {\n" \
"	const float MAX_UINT = 4294967295.0; // (2^16-1)\n" \
"	float random = state / MAX_UINT;\n" \
"	hash(state);\n" \
"	return random;\n" \
"}\n" \
"\n" \
"ivec4 speciesMask(uint species) {\n" \
"	switch(species) {\n" \
"	case 0: return ivec4(1, 0, 0, 0);\n" \
"	case 1: return ivec4(0, 1, 0, 0);\n" \
"	case 2: return ivec4(0, 0, 1, 0);\n" \
"	case 3: return ivec4(0, 0, 0, 1);\n" \
"	}\n" \
"}\n" \
"\n" \
"float sense(vec2 pos, ivec2 size, ivec4 speciesMask) {\n" \
"	ivec2 imageCoords = ivec2(pos * size);\n" \
"	float sensed = 0;\n" \
"	for(int x = -1; x <= 1; ++x)\n" \
"		for(int y = -1; y <= 1; ++y)\n" \
"			sensed += dot(imageLoad(image, imageCoords + ivec2(x, y)), speciesMask);\n" \
"	return sensed;\n" \
"}\n" \
"\n" \
"void move(inout Agent agent, Species species, ivec2 size, ivec4 speciesMask) {\n" \
"	float angle = agent.angleRadians;\n" \
"	float spacing = species.sensorSpacingRadians;\n" \
"	vec2 dir = {cos(angle), sin(angle)};\n" \
"	vec2 dirLeft = {cos(angle + spacing), sin(angle + spacing)};\n" \
"	vec2 dirRight = {cos(angle - spacing), sin(angle - spacing)};\n" \
"	vec2 pos = agent.pos;\n" \
"	float distance = species.sensorDistance;\n" \
"	speciesMask = speciesMask * 2 - 1;\n" \
"	float sensed = sense(pos + dir * distance, size, speciesMask);\n" \
"	float sensedLeft = sense(pos + dirLeft * distance, size, speciesMask);\n" \
"	float sensedRight = sense(pos + dirRight * distance, size, speciesMask);\n" \
"	float turnAmount = species.turnRadiansPerSecond * deltaTime;\n" \
"	if(sensedLeft > sensed && sensedLeft > sensedRight) {\n" \
"		agent.angleRadians += turnAmount;\n" \
"		dir = dirLeft;\n" \
"	} else if(sensedRight > sensed && sensedRight > sensedLeft) {\n" \
"		agent.angleRadians -= turnAmount;\n" \
"		dir = dirRight;\n" \
"	}\n" \
"	float moveDistance = species.moveSpeed * deltaTime;\n" \
"	agent.pos += dir * moveDistance;\n" \
"}\n" \
"\n" \
"void main() {\n" \
"	uint index = gl_GlobalInvocationID.x;\n" \
"	if(index >= numAgents)\n" \
"		return;\n" \
"	Agent agent = agents[index];\n" \
"	Species species = species[agent.species];\n" \
"	ivec2 size = imageSize(image);\n" \
"	ivec4 speciesMask = speciesMask(agent.species);\n" \
"	move(agent, species, size, speciesMask);\n" \
"	if(agent.pos.x < 0 || agent.pos.y < 0 || agent.pos.x > 1 || agent.pos.y > 1) {\n" \
"		agent.pos = clamp(agent.pos, 0, 1);\n" \
"		uint state = randomState(index);\n" \
"		agent.angleRadians = random01(state) * 2 * PI; // new random angle\n" \
"	}\n" \
"	agents[index] = agent;\n" \
"	imageStore(image, ivec2(agent.pos * size), speciesMask);\n" \
"}\n" \
""
#define VERTEX_GLSL \
"#version 460\n" \
"\n" \
"layout(location = 0) in vec2 inPos;\n" \
"out vec2 uvCoords;\n" \
"\n" \
"void main() {\n" \
"	gl_Position = vec4(inPos, 0, 1);\n" \
"	uvCoords = (inPos + 1) / 2;\n" \
"}\n" \
""
