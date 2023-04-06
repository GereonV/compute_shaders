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
"layout(location = 0) uniform float uDeltaTime; // must be set\n" \
"layout(location = 1) uniform float uDecayRate = 1; // must be non-negative\n" \
"layout(location = 2) uniform float uDiffuseRate = 1; // must be non-negative\n" \
"layout(binding = 0, rgba32f) uniform image2D uImage;\n" \
"\n" \
"void main() {\n" \
"	ivec2 size = imageSize(uImage);\n" \
"	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);\n" \
"	if(pos.x >= size.x || pos.y >= size.y)\n" \
"		return;\n" \
"	vec4 original = imageLoad(uImage, pos);\n" \
"	vec4 sum = vec4(0);\n" \
"	for(int x = -1; x <= 1; ++x)\n" \
"		for(int y = -1; y <= 1; ++y)\n" \
"			sum += imageLoad(uImage, clamp(ivec2(x, y) + pos, ivec2(0), size));\n" \
"	vec4 blurred = sum / 9;\n" \
"	vec4 diffused = mix(original, blurred, uDiffuseRate * uDeltaTime);\n" \
"	vec4 decayed = max(diffused - uDecayRate * uDeltaTime, 0);\n" \
"	imageStore(uImage, pos, decayed);\n" \
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
