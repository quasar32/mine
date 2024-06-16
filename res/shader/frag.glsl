#version 330

uniform sampler2D tex;

in vec2 uv;

out vec4 rgba;

void main() {
	rgba = texture(tex, uv); 
}
