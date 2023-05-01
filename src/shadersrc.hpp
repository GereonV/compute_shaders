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
#define RT_GLSL \
"#version 460\n" \
"\n" \
"layout(local_size_x = 8, local_size_y = 8) in;\n" \
"\n" \
"struct Sphere {\n" \
"	vec4 data; // (center.xyz, radius)\n" \
"};\n" \
"\n" \
"layout(location = 0) uniform float fov = 1;\n" \
"layout(binding = 0, rgba32f) uniform image2D image;\n" \
"layout(binding = 0, std430) buffer _block_name {\n" \
"	Sphere spheres[];\n" \
"};\n" \
"\n" \
"// Circle defined by c & r: p: length(p-c)²=r²\n" \
"// Ray defined by o & d: p: p=o+l*d\n" \
"// length(o+l*d-c)²=r²\n" \
"// =(o.x+l*d.x-c.x)²+(o.y+l*d.y-c.y)²\n" \
"// =o.x²+2*o.x*l*d.x+l²*d.x²-2*(o.x+l*d.x)*c.x+c.x²+o.y²+2*o.y*l*d.y+l²*d.y²-2*(o.y+l*d.y)*c.y+c.y²\n" \
"// =l²(d.x²+d.y²)+l*2*(d.x*(o.x-c.x)+d.y*(o.y-c.y))+(o.x²-2*o.x*c.x+c.x²+o.y²-2*o.y*c.y+c.y²)\n" \
"// =l²length(d)²+l*2*dot(d,o-c)+length(o-c)²\n" \
"// <=>l²length(d)²+l*2*dot(d,o-c)+length(o-c)²-r²\n" \
"// length(d)²=1;p=2*dot(d,o-c);q=length(o-c)²-r²\n" \
"// l=p+sqrt(p²-4q)/-2\n" \
"float distanceToSphere(vec3 o, vec3 d, vec3 c, float r) {\n" \
"	vec3 co = o - c;\n" \
"	float p = 2 * dot(d, o - c);\n" \
"	float q = dot(co, co) - r * r;\n" \
"	float discriminator = p * p - 4 * q;\n" \
"	if(discriminator < 0)\n" \
"		return -1;\n" \
"	float numerator = p + sqrt(discriminator);\n" \
"	if(numerator > 0)\n" \
"		return -1;\n" \
"	return -numerator/2;\n" \
"}\n" \
"\n" \
"void main() {\n" \
"	ivec2 size = imageSize(image);\n" \
"	if(gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y)\n" \
"		return;\n" \
"	ivec2 iC = ivec2(gl_GlobalInvocationID.xy);\n" \
"	vec2 uv = iC / vec2(size - 1);\n" \
"\n" \
"	vec3 origin = vec3(0);\n" \
"	float maxX = tan(fov / 2);\n" \
"	vec2 max = {maxX, maxX * size.y / size.x};\n" \
"	vec3 dir = normalize(vec3((2 * uv - 1) * max, -1));\n" \
"	vec4 s = spheres[0].data;\n" \
"	vec3 c = s.xyz;\n" \
"	float r = s.w;\n" \
"\n" \
"	float d = distanceToSphere(origin, dir, c, r);\n" \
"	vec3 col = d == -1 ? vec3(0) : abs(normalize(origin + d * dir - c));\n" \
"	imageStore(image, iC, vec4(col, 1));\n" \
"}\n" \
""
#define SLIME_GLSL \
"#version 460\n" \
"\n" \
"layout(local_size_x = 1) in;\n" \
"\n" \
"struct Agent {\n" \
"	vec2 pos;\n" \
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
"layout(location = 3) uniform uint overlapping;\n" \
"layout(location = 4) uniform Species _species[4];\n" \
"layout(binding = 0, rgba32f) uniform image2D image;\n" \
"layout(binding = 0, std430) buffer _block_name {\n" \
"	Agent agents[];\n" \
"};\n" \
"\n" \
"ivec4 _speciesMask(uint species) {\n" \
"	switch(species) {\n" \
"	case 0: return ivec4(1, 0, 0, 0);\n" \
"	case 1: return ivec4(0, 1, 0, 0);\n" \
"	case 2: return ivec4(0, 0, 1, 0);\n" \
"	case 3: return ivec4(0, 0, 0, 1);\n" \
"	}\n" \
"}\n" \
"\n" \
"const uint index = gl_GlobalInvocationID.x;\n" \
"const ivec2 size = imageSize(image);\n" \
"const ivec4 speciesMask = _speciesMask(agents[index].species);\n" \
"const ivec4 speciesMult = speciesMask * 2 - 1;\n" \
"const Species species = _species[agents[index].species];\n" \
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
"uint randomState() {\n" \
"	uint idx = index;\n" \
"	uint micros = uint(time * 1000000);\n" \
"	hash(idx);\n" \
"	hash(micros);\n" \
"	return idx + micros;\n" \
"}\n" \
"\n" \
"float random01(inout uint state) {\n" \
"	const float MAX_UINT = 4294967295.0; // (2^16-1)\n" \
"	float random = state / MAX_UINT;\n" \
"	hash(state);\n" \
"	return random;\n" \
"}\n" \
"\n" \
"float sense(vec2 pos, float angle) {\n" \
"	vec2 dir = {cos(angle), sin(angle)};\n" \
"	ivec2 sensorPos = ivec2(pos + dir * species.sensorDistance);\n" \
"	float sensed = 0;\n" \
"	for(int x = -1; x <= 1; ++x)\n" \
"		for(int y = -1; y <= 1; ++y)\n" \
"			sensed += dot(imageLoad(image, sensorPos + ivec2(x, y)), speciesMult);\n" \
"	return sensed;\n" \
"}\n" \
"\n" \
"void move(inout Agent agent, inout uint state) {\n" \
"	vec2 pos = agent.pos;\n" \
"	float angle = agent.angleRadians;\n" \
"	float spacing = species.sensorSpacingRadians;\n" \
"	float sensed = sense(pos, angle);\n" \
"	float sensedLeft = sense(pos, angle + spacing);\n" \
"	float sensedRight = sense(pos, angle - spacing);\n" \
"	float turnAmount = species.turnRadiansPerSecond * deltaTime;\n" \
"	if(sensedLeft > sensed && sensedLeft > sensedRight)\n" \
"		agent.angleRadians += turnAmount;\n" \
"	else if(sensedRight > sensed && sensedRight > sensedLeft)\n" \
"		agent.angleRadians -= turnAmount;\n" \
"	agent.angleRadians += (2 * random01(state) - 1) * PI / 2 * deltaTime;\n" \
"	vec2 dir = {cos(agent.angleRadians), sin(agent.angleRadians)};\n" \
"	float moveDistance = species.moveSpeed * deltaTime;\n" \
"	agent.pos += dir * moveDistance;\n" \
"}\n" \
"\n" \
"void main() {\n" \
"	if(index >= numAgents)\n" \
"		return;\n" \
"	Agent agent = agents[index];\n" \
"	uint state = randomState();\n" \
"	move(agent, state);\n" \
"	if(agent.pos.x < 0 || agent.pos.y < 0 || agent.pos.x > size.x || agent.pos.y > size.y) {\n" \
"		agent.pos = clamp(agent.pos, vec2(0), size);\n" \
"		agent.angleRadians = random01(state) * 2 * PI; // new random angle\n" \
"	}\n" \
"	agents[index] = agent;\n" \
"	ivec2 imageCoords = ivec2(agent.pos);\n" \
"	vec4 trail = overlapping == 1 ? max(speciesMask + imageLoad(image, imageCoords), 1) : speciesMask;\n" \
"	imageStore(image, imageCoords, trail);\n" \
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
