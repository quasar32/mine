#version 330

#define X(v) (((v)       ) & 31U)
#define Y(v) (((v) >>  5U) & 31U)
#define Z(v) (((v) >> 10U) & 31U)
#define U(v) (((v) >> 15U) & 31U)
#define V(v) (((v) >> 20U) & 31U)
#define L(v) (((v) >> 25U) &  1U)

layout(location=0) in uint vert;

uniform mat4 world;

out vec2 uv;
out float dist;
flat out uint select;

void main() {
	vec3 xyz;
	vec4 xyzw;
	xyz = vec3(X(vert), Y(vert), Z(vert));
	xyzw = vec4(xyz - 0.5F, 1.0F);
	uv = vec2(U(vert), V(vert)) / 16.0F; 
	gl_Position = world * xyzw; 
	dist = gl_Position.w;
	select = L(vert);
}
