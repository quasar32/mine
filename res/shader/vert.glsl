#version 330

layout(location=0) in uvec4 vertex;

uniform mat4 world;

out vec2 uv;

void main() {
	vec4 xyzw;
	uint tile;
	xyzw = vec4(vec3(vertex) - 0.5F, 1.0F);
	tile = vertex[3];
	uv = vec2(tile & 15u, tile >> 4u) / 16.0F;
	gl_Position = world * xyzw; 
}
