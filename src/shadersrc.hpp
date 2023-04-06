#define COMPUTE_GLSL \
"#version 460\n" \
"\n" \
"layout(local_size_x = 1) in;\n" \
"\n" \
"layout(binding = 0, rgba32f) uniform image2D uImage;\n" \
"\n" \
"// layout(std430, binding = 0) buffer block_name {\n" \
"// 	vec4 pixels[]; // vec3 are aligned as vec4 anyway\n" \
"// };\n" \
"\n" \
"void main() {\n" \
"	ivec2 size = imageSize(uImage);\n" \
"	if(gl_WorkGroupID.x >= size.x || gl_WorkGroupID.y >= size.y)\n" \
"		return;\n" \
"	vec2 uvCoords = vec2(gl_WorkGroupID) / (gl_NumWorkGroups.xy - 1);\n" \
"	ivec2 imageCoords = ivec2(gl_WorkGroupID.xy);\n" \
"	imageStore(uImage, imageCoords, vec4(uvCoords, 0, 1));\n" \
"\n" \
"	// uint pixelIndex = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;\n" \
"	// pixels[pixelIndex] = vec4(uvCoords, 0, 1);\n" \
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
