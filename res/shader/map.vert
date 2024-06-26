#version 330

#define X(v) (((v)       ) & 31U)
#define Y(v) (((v) >>  5U) & 31U)
#define Z(v) (((v) >> 10U) & 31U)
#define U(v) (((v) >> 15U) & 15U)
#define V(v) (((v) >> 19U) & 15U)
#define L(v) (((v) >> 23U) & 15U)
#define F(v) (((v) >> 27U) & 15U)

layout(location=0) in uint vert;

uniform mat4 world;

out vec2 uv;
out float dist;
out float light;

void main() {
	vec3 xyz;
	vec4 xyzw;
	xyz = vec3(X(vert), Y(vert), Z(vert));
	xyzw = vec4(xyz - 0.5F, 1.0F);
	uv = vec2(U(vert), V(vert)) / 16.0F; 
	gl_Position = world * xyzw; 
	dist = gl_Position.w;
	light = L(vert) * F(vert) / 150.0F;
}
