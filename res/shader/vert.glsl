#version 330

layout(location=0) in vec3 pos;
layout(location=1) in vec3 nor;
layout(location=2) in vec2 tex;
layout(location=3) in uvec3 off;
layout(location=4) in uint tile;

uniform mat4 proj;
uniform mat4 view;

out vec2 vs_tex;
flat out vec2 vs_uv;

void main() {
	vs_tex = tex;
	vs_uv = vec2(tile & 15u, tile >> 4u);
	gl_Position = proj * view * (vec4(pos + off, 1.0F)); 
}
