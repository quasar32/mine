#version 330

uniform sampler2D tex;
uniform int tile;

in vec2 vs_tex;

out vec4 rgba;

void main() {
	vec2 uv;
	uv = vec2(tile % 16, tile / 16);
	uv = (uv + vs_tex) / 16.0F;
	rgba = texture(tex, uv); 
}
