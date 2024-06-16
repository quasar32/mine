#include <cglm/cglm.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stb_image.h>
#include <math.h>

#define MAX_PITCH (GLM_PI_2f - 0.01F)
#define MIN_PITCH (-MAX_PITCH)
#define SENSITIVITY 0.1F

#define CHUNK_LEN 128 
#define MAX_VERTICES (36 * CHUNK_LEN * CHUNK_LEN * CHUNK_LEN) 

#define BACK     1U
#define FRONT    2U
#define LEFT     4U
#define RIGHT    8U
#define BOTTOM  16U
#define TOP     32U

#define FOR_XYZ \
	for (x = 0; x < CHUNK_LEN; x++) \
		for (y = 0; y < CHUNK_LEN; y++) \
			for (z = 0; z < CHUNK_LEN; z++) 

struct vertex {
	uint8_t x;
	uint8_t y;
	uint8_t z;
	uint8_t t;
};

static struct vertex cube[6][6] = {
	{
		{1, 1, 0, 17},
		{1, 0, 0, 1},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 1, 0, 16},
		{1, 1, 0, 17},
	},
	{
		{1, 1, 1, 16},
		{0, 1, 1, 17},
		{0, 0, 1, 1},
		{0, 0, 1, 1},
		{1, 0, 1, 0},
		{1, 1, 1, 16},
	},
	{
		{0, 1, 1, 16},
		{0, 1, 0, 17},
		{0, 0, 0, 1},
		{0, 0, 0, 1},
		{0, 0, 1, 0},
		{0, 1, 1, 16},
	},
	{
		{1, 1, 1, 17},
		{1, 0, 1, 1},
		{1, 0, 0, 0},
		{1, 0, 0, 0},
		{1, 1, 0, 16},
		{1, 1, 1, 17},
	},
	{
		{1, 0, 1, 16},
		{0, 0, 1, 17},
		{0, 0, 0, 1},
		{0, 0, 0, 1},
		{1, 0, 0, 0},
		{1, 0, 1, 16},
	},
	{
		{1, 1, 1, 0},
		{1, 1, 0, 16},
		{0, 1, 0, 17},
		{0, 1, 0, 17},
		{0, 1, 1, 1},
		{1, 1, 1, 0},
	}
};

static uint8_t map[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
static uint8_t faces[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
static struct vertex vertices[MAX_VERTICES];
static int nvertices;

static int width = 640;
static int height = 480;

static vec3 eye = {0.0F, 4.0F, 0.0F};
static vec3 front = {0.0F, 0.0F, -1.0F};
static vec3 up = {0.0F, 1.0F, 0.0F};
static vec3 right;

static float dt;

[[noreturn]]
static void die(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

static char *fread_all_str(FILE *f) {
	char *buf, *tmp;
	size_t size;

	buf = NULL;
	size = 0;
	while ((tmp = realloc(buf, size + 4096))) {
		buf = tmp;
		size += fread(buf + size, 1, 4096, f);
		if (ferror(f)) {
			break;
		}
		if (feof(f)) {
			buf[size] = '\0';
			return buf;
		}
	}
	free(buf);
	return NULL;
}

static char *read_all_str(const char *path) {
	FILE *f;
	char *buf;

	f = fopen(path, "rb");
	if (!f) {
		return NULL;
	}
	buf = fread_all_str(f);
	fclose(f);
	return buf;
}

static GLuint load_shader(GLenum type, const char *path) {
	GLuint shader;
	int status;
	char log[1024];
	const char *src;

	src = read_all_str(path);
	if (!src) {
		die("load_shader: %s\n", strerror(errno));
	}
	shader = glCreateShader(type); 
	glShaderSource(shader, 1, &src, NULL);
	free((void *) src);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status) {
		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		die("load_shader: %s\n", log);
	}
	return shader;
}

static GLuint load_prog(GLuint vs, GLuint fs) {
	GLuint prog;
	int status;
	char buf[1024];

	prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glDetachShader(prog, vs);
	glDetachShader(prog, fs);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (!status) {
		glGetProgramInfoLog(prog, sizeof(buf), NULL, buf);
		die("load_prog: %s\n", buf);
	}
	return prog;
}

static void glfw_die(const char *fun) {
	const char *msg;
	int err;

	err = glfwGetError(&msg);
	die("%s (%d): %s\n", fun, err, msg);
}

static void resize_cb(GLFWwindow *wnd, int w, int h) {
	glViewport(0, 0, w, h);
	width = w;
	height = h;
}

static char *stpecpy(char *dst, char *end, const char *src) {
	if (dst == end) {
		return end;
	}
	do {
		*dst = *src++;
		if (!*dst) {
			return dst;
		}
	} while (++dst < end);
	end[-1] = '\0';
	return end;
}

static void set_data_path(void) {
	char path[PATH_MAX];
	char *nul, *end;

	if (!realpath("/proc/self/exe", path)) {
		die("realpath(): %s\n", strerror(errno));
	}
	nul = strrchr(path, '/');
	if (!nul) {
		die("path missing slash\n");
	}
	end = path + PATH_MAX; 
	nul = stpecpy(nul + 1, end, "res");
	if (nul == end) {
		die("path too long\n");
	}
	if (chdir(path) < 0) {
		die("chdir(): %s\n", strerror(errno));
	}
}

static float clamp(float v, float l, float h) {
	return fminf(fmaxf(v, l), h);
}

static void mouse_cb(GLFWwindow *wnd, double x, double y) {
	static int first;
	static double last_x;
	static double last_y;
	static float yaw = -GLM_PI_2f; 
	static float pitch;
	float off_x;
	float off_y;

	if (!first) {
		last_x = x;
		last_y = y;
		first = 1;
	}
	off_x = x - last_x;
	off_y = last_y - y;
	last_x = x;
	last_y = y;
	off_x *= dt * SENSITIVITY;
	off_y *= dt * SENSITIVITY;
	yaw += off_x;
	pitch += off_y;
	yaw = fmodf(yaw, GLM_PIf * 2.0F);
	pitch = clamp(pitch, MIN_PITCH, MAX_PITCH);
	front[0] = cosf(yaw) * cosf(pitch);
	front[1] = sinf(pitch);
	front[2] = sinf(yaw) * cosf(pitch);
	glm_normalize(front);
}

static void add_vertex(int x, int y, int z) {
	struct vertex *v;
	int i, j;

	for (i = 0; i < 6; i++) {
		if ((faces[x][y][z] >> i) & 1) {
			for (j = 0; j < 6; j++) {
				v = &vertices[nvertices++];
				v->x = x + cube[i][j].x;
				v->y = y + cube[i][j].y;
				v->z = z + cube[i][j].z;
				v->t = cube[i][j].t; 
			}
		}
	}
}

static void create_vertices(void) {
	int x, y, z;

	FOR_XYZ {
		map[x][y][z] = !!(rand() % 4);
	}
	FOR_XYZ {
		if (!map[x][y][z]) {
			continue;
		}
		faces[x][y][z] = FRONT | RIGHT | TOP; 
		if (x == 0) {
			faces[x][y][z] |= LEFT;
		} else if (map[x - 1][y][z]) {
			faces[x - 1][y][z] &= ~RIGHT;
		} else {
			faces[x][y][z] |= LEFT;
		}
		if (y == 0) {
			faces[x][y][z] |= BOTTOM;
		} else if (map[x][y - 1][z]) {
			faces[x][y - 1][z] &= ~TOP;
		} else {
			faces[x][y][z] |= BOTTOM;
		}
		if (z == 0) {
			faces[x][y][z] |= BACK;
		} else if (map[x][y][z - 1]) {
			faces[x][y][z - 1] &= ~FRONT;
		} else {
			faces[x][y][z] |= BACK;
		}
	}
	FOR_XYZ {
		if (map[x][y][z]) {
			add_vertex(x, y, z);
		}
	}
}

int main(void) {
	GLFWwindow *wnd;
	GLuint vs, fs;
	GLuint prog;
	GLuint vao, vbo;
	GLint proj_loc;
	GLint view_loc;
	GLint tex_loc;
	mat4 proj, view;
	stbi_uc *data;
	GLuint tex;
	int w, h, channels;
	double t0, t1;
	vec3 center;

	create_vertices();

	if (!glfwInit()) {
		glfw_die("glfwInit");
	}
	atexit(glfwTerminate);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	wnd = glfwCreateWindow(width, height, "Mine", NULL, NULL);
	if (!wnd) {
		glfw_die("glfwCreateWindow");
	}
	glfwMakeContextCurrent(wnd);
	if (!gladLoadGL((GLADloadfunc) glfwGetProcAddress)) {
		glfw_die("glfwGetProcAddress");
	}
	set_data_path();
	data = stbi_load("img/atlas.png", &w, &h, &channels, 3);
	if (!data) {
		die("stbi_load: %s\n", stbi_failure_reason());
	}
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
			GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 
			0, GL_RGB, GL_UNSIGNED_BYTE, data); 
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	data = NULL;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), 
			vertices, GL_STATIC_DRAW);
	glVertexAttribIPointer(0, 4, GL_UNSIGNED_BYTE, 4, NULL);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	vs = load_shader(GL_VERTEX_SHADER, "shader/vert.glsl"); 
	fs = load_shader(GL_FRAGMENT_SHADER, "shader/frag.glsl");
	prog = load_prog(vs, fs);
	proj_loc = glGetUniformLocation(prog, "proj");
	view_loc = glGetUniformLocation(prog, "view");
	tex_loc = glGetUniformLocation(prog, "tex");
	glfwSetFramebufferSizeCallback(wnd, resize_cb);
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(wnd, mouse_cb);
	glfwSwapInterval(1);
	glfwShowWindow(wnd);
	t0 = glfwGetTime();

	float total = 0.0F;
	int nframes = 0;
	while (!glfwWindowShouldClose(wnd)) {
		glfwPollEvents();
		t1 = glfwGetTime();
		dt = t1 - t0;
		t0 = t1;
		nframes++;
		total += dt;
		fflush(stdout);
		glm_cross(front, up, right);
		glm_normalize(right);
		if (glfwGetKey(wnd, GLFW_KEY_W)) {
			glm_vec3_muladds(front, 8.0F * dt, eye);
		}
		if (glfwGetKey(wnd, GLFW_KEY_S)) {
			glm_vec3_mulsubs(front, 8.0F * dt, eye);
		}
		if (glfwGetKey(wnd, GLFW_KEY_A)) {
			glm_vec3_mulsubs(right, 8.0F * dt, eye);
		}
		if (glfwGetKey(wnd, GLFW_KEY_D)) {
			glm_vec3_muladds(right, 8.0F * dt, eye);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm_perspective(GLM_PI_4f, width / (float) height, 
				0.01F, 100.0F, proj); 
		glm_mat4_identity(view);
		glm_vec3_add(eye, front, center);
		glm_lookat(eye, center, up, view);
		glUseProgram(prog);
		glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *) proj);
		glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float *) view);
		glBindVertexArray(vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glUniform1i(tex_loc, 0);
		glDrawArrays(GL_TRIANGLES, 0, nvertices);
		glfwSwapBuffers(wnd);
	}
	printf("%f\n", nframes / total);
	return EXIT_SUCCESS;
}
