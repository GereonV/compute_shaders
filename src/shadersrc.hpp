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
"layout(location = 0) uniform double deltaTime; // must be set\n" \
"layout(location = 1) uniform float decayRate = 0.1; // must be non-negative\n" \
"layout(location = 2) uniform float diffuseRate = 3; // must be non-negative\n" \
"layout(binding = 0, rgba32f) uniform image2D image;\n" \
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
"}\n" \
""
#define SLIME_GLSL \
"#version 460\n" \
"\n" \
"layout(local_size_x = 1) in;\n" \
"\n" \
"struct Agent {\n" \
"	vec2 pos;\n" \
"	float angle;\n" \
"};\n" \
"\n" \
"#define PI 3.1415926535897932384626433832795\n" \
"\n" \
"layout(location = 0) uniform double time; // as seed\n" \
"layout(location = 1) uniform double deltaTime; // must be set\n" \
"layout(location = 2) uniform float moveSpeed = 0.15;\n" \
"layout(location = 3) uniform float turnSpeed = PI;\n" \
"layout(location = 4) uniform float sensorAngle = PI / 6;\n" \
"layout(location = 5) uniform float sensorDistance = 35;\n" \
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
"	float random = state / 4294967295.0;\n" \
"	hash(state);\n" \
"	return random;\n" \
"}\n" \
"\n" \
"void main() {\n" \
"	uint index = gl_GlobalInvocationID.x;\n" \
"	uint numAgents = agents.length();\n" \
"	if(index >= numAgents)\n" \
"		return;\n" \
"	Agent agent = agents[index];\n" \
"	vec2 dir = {cos(agent.angle), sin(agent.angle)};\n" \
"	float moveDistance = moveSpeed * float(deltaTime);\n" \
"	vec2 newPos = agent.pos + dir * moveDistance;\n" \
"	uint state = randomState(index);\n" \
"	if(newPos.x < 0 || newPos.y < 0 || newPos.x > 1 || newPos.y > 1) {\n" \
"		newPos = clamp(newPos, 0, 1);\n" \
"		agents[index].angle = random01(state) * 2 * PI;\n" \
"	} else {\n" \
"		float turn = turnSpeed * random01(state) * float(deltaTime);\n" \
"		// agents[index].angle += turn;\n" \
"	}\n" \
"	agents[index].pos = newPos;\n" \
"	ivec2 size = imageSize(image);\n" \
"	ivec2 imageCoords = ivec2(newPos * (size + 0.99999999));\n" \
"	imageStore(image, imageCoords, vec4(1));\n" \
"	ivec2 center = imageSize(image) / 2;\n" \
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
