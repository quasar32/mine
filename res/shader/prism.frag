#version 330

uniform sampler2D tex;

in vec2 vert_uv;
in float vert_lum;

out vec4 rgba;

void main(void) {
	rgba = texture(tex, vert_uv) * vec4(vec3(vert_lum), 1.0F);
}
