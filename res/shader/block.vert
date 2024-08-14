#version 330

uniform mat4 world;

layout(location=0) in uint v;

#define X ((v) & 31U)
#define Y (((v) >> 5U) & 31U)
#define Z (((v) >> 10U) & 31U)
#define U (((v) >> 15U) & 31U)
#define V (((v) >> 20U) & 31U)
#define L (((v) >> 25U) & 31U)

out vec2 vert_uv;
out float vert_lum;

void main(void) {
	vert_uv = vec2(U, V) / 16.0F; 
	vert_lum = float(L) / 16.0F; 
	gl_Position = world * vec4(X, Y, Z, 1.0F); 
}
