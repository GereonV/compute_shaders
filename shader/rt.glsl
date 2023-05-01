#version 460

layout(local_size_x = 8, local_size_y = 8) in;

struct Sphere {
	vec4 data; // (center.xyz, radius)
};

layout(location = 0) uniform float fov = 1;
layout(binding = 0, rgba32f) uniform image2D image;
layout(binding = 0, std430) buffer _block_name {
	Sphere spheres[];
};

// Circle defined by c & r: p: length(p-c)²=r²
// Ray defined by o & d: p: p=o+l*d
// length(o+l*d-c)²=r²
// =(o.x+l*d.x-c.x)²+(o.y+l*d.y-c.y)²
// =o.x²+2*o.x*l*d.x+l²*d.x²-2*(o.x+l*d.x)*c.x+c.x²+o.y²+2*o.y*l*d.y+l²*d.y²-2*(o.y+l*d.y)*c.y+c.y²
// =l²(d.x²+d.y²)+l*2*(d.x*(o.x-c.x)+d.y*(o.y-c.y))+(o.x²-2*o.x*c.x+c.x²+o.y²-2*o.y*c.y+c.y²)
// =l²length(d)²+l*2*dot(d,o-c)+length(o-c)²
// <=>l²length(d)²+l*2*dot(d,o-c)+length(o-c)²-r²
// length(d)²=1;p=2*dot(d,o-c);q=length(o-c)²-r²
// l=p+sqrt(p²-4q)/-2
float distanceToSphere(vec3 o, vec3 d, vec3 c, float r) {
	vec3 co = o - c;
	float p = 2 * dot(d, o - c);
	float q = dot(co, co) - r * r;
	float discriminator = p * p - 4 * q;
	if(discriminator < 0)
		return -1;
	float numerator = p + sqrt(discriminator);
	if(numerator > 0)
		return -1;
	return -numerator/2;
}

void main() {
	ivec2 size = imageSize(image);
	if(gl_GlobalInvocationID.x >= size.x || gl_GlobalInvocationID.y >= size.y)
		return;
	ivec2 iC = ivec2(gl_GlobalInvocationID.xy);
	vec2 uv = iC / vec2(size - 1);

	vec3 origin = vec3(0);
	float maxX = tan(fov / 2);
	vec2 max = {maxX, maxX * size.y / size.x};
	vec3 dir = normalize(vec3((2 * uv - 1) * max, -1));
	vec4 s = spheres[0].data;
	vec3 c = s.xyz;
	float r = s.w;

	float d = distanceToSphere(origin, dir, c, r);
	vec3 col = d == -1 ? vec3(0) : abs(normalize(origin + d * dir - c));
	imageStore(image, iC, vec4(col, 1));
}
