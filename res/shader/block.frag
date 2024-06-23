#version 330

uniform sampler2D tex;

in vec2 uv;

out vec4 rgba;
in float dist;
flat in uint select;

void main() {
	float factor;
	float white;
	vec4 fog;
	fog = vec4(0.5F, 0.6F, 1.0F, 1.0F);
	factor = (100.0F - dist) / 99.9F;
	factor = clamp(factor, 0.0, 1.0);
	white = select != 0U ? 1.2F : 1.0F;
	rgba = mix(fog, texture(tex, uv) * white, factor); 
}
