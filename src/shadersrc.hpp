#define FRAGMENT_GLSL \
"#version 460\n" \
"\n" \
"in vec2 uvCoords;\n" \
"out vec4 outColor;\n" \
"\n" \
"void main() {\n" \
"	outColor = vec4(uvCoords.xy, 0, 1);\n" \
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
