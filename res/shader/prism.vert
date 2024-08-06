#version 330

uniform mat4 world;

layout(location=0) in vec3 xyz;
layout(location=1) in vec2 uv;
layout(location=2) in float lum;

out vec2 vert_uv;
out float vert_lum;

void main(void) {
	vert_uv = uv; 
	vert_lum = lum; 
	gl_Position = world * vec4(xyz, 1.0F); 
}
