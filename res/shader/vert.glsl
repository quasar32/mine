#version 330

layout(location=0) in uvec4 vertex;

uniform mat4 proj;
uniform mat4 view;

out vec2 uv;

void main() {
	vec4 xyzw;
	uint tile;
	xyzw = vec4(vec3(vertex), 1.0F);
	tile = vertex[3];
	uv = vec2(tile & 15u, tile >> 4u) / 16.0F;
	gl_Position = proj * view * xyzw; 
}
