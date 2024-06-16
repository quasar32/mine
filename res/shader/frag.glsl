#version 330

uniform sampler2D tex;
uniform int tile;

in vec2 vs_tex;
flat in vec2 vs_uv;

out vec4 rgba;

void main() {
	vec2 uv;
	uv = (vs_uv + vs_tex) / 16.0F;
	rgba = texture(tex, uv); 
}
