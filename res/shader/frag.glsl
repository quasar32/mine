#version 330

uniform sampler2D tex;

in vec2 vs_tex;

out vec4 rgba;

void main() {
	rgba = texture(tex, vs_tex);
	//rgba = vec4(1.0F, 0.0F, 1.0F, 1.0F);
}
