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

#define COUNT_OF(ary) (sizeof(ary) / sizeof(*(ary)))
#define END_OF(ary) ((ary) + COUNT_OF((ary)))

#define MAX_PITCH (GLM_PI_2f - 0.01F)
#define MIN_PITCH (-MAX_PITCH)
#define SENSITIVITY 0.1F

struct vertex {
	vec3 pos;
	vec3 nor;
	vec2 tex;
};

struct instance {
	vec3 off;
	int tile;
};

#define CHUNK 64

static struct instance instances[CHUNK][CHUNK][CHUNK];

static uint8_t indices[] = {
	/* back */
	0, 1, 2,
	2, 3, 0,

	/* front */
	4, 5, 6,
	6, 7, 4,

	/* left */
	8, 9, 10,
	10, 11, 8,

	/* right */
	12, 13, 14,
	14, 15, 12,
	
	/* bottom */
	16, 17, 18,
	18, 19, 16,

	/* top */
	20, 21, 22,
	22, 23, 20
};

static struct vertex vertices[] = {
	/* back */
	{{0.5F, 0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {0.0F, 0.0F}},
	{{0.5F, -0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {0.0F, 1.0F}},
	{{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {1.0F, 1.0F}},
	{{-0.5F, 0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}, {1.0F, 0.0F}},

	/* front */
	{{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
	{{-0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
	{{-0.5F, -0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
	{{0.5F, -0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},

	/* left */
	{{-0.5F, 0.5F, 0.5F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},
	{{-0.5F, 0.5F, -0.5F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
	{{-0.5F, -0.5F, -0.5F}, {-1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
	{{-0.5F, -0.5F, 0.5F}, {-1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},

	/* right */
	{{0.5F, 0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F}},
	{{0.5F, -0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F}},
	{{0.5F, -0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F}},
	{{0.5F, 0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}, {1.0F, 0.0F}},

	/* bottom */
	{{0.5F, -0.5F, 0.5F}, {0.0F, -1.0F, 0.0F}, {1.0F, 0.0F}},
	{{-0.5F, -0.5F, 0.5F}, {0.0F, -1.0F, 0.0F}, {0.0F, 0.0F}},
	{{-0.5F, -0.5F, -0.5F}, {0.0F, -1.0F, 0.0F}, {0.0F, 1.0F}},
	{{0.5F, -0.5F, -0.5F}, {0.0F, -1.0F, 0.0F}, {1.0F, 1.0F}},

	/* top */
	{{0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}, {1.0F, 1.0F}},
	{{0.5F, 0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}, {1.0F, 0.0F}},
	{{-0.5F, 0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
	{{-0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}, {0.0F, 1.0F}},
};

static int width = 640;
static int height = 480;

static vec3 eye = {0.0F, 0.0F, 10.0F};
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
	end = END_OF(path);
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

int main(void) {
	GLFWwindow *wnd;
	GLuint vs, fs;
	GLuint prog;
	GLuint vao, bos[3];
	GLint proj_loc;
	GLint view_loc;
	GLint tex_loc;
	mat4 proj, view;
	stbi_uc *data;
	GLuint tex;
	int w, h, channels;
	double t0, t1;
	vec3 center;

	for (int i = 0; i < CHUNK; i++) {
		for (int j = 0; j < CHUNK; j++) {
			for (int k = 0; k < CHUNK; k++) {
				struct instance *instance;
				instance = &instances[i][j][k];
				instance->off[0] = i;
				instance->off[1] = j;
				instance->off[2] = k;
				instance->tile = 4;
			}
		}
	}
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
	glGenBuffers(3, bos);
	glBindBuffer(GL_ARRAY_BUFFER, bos[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), 
			vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 
			sizeof(*vertices), NULL);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(*vertices), 
			(void *) offsetof(struct vertex, tex));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices),
			indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, bos[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(instances),
			instances, GL_STATIC_DRAW);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(***instances), 
			(void *) offsetof(struct instance, off));
	glEnableVertexAttribArray(3);
	glVertexAttribDivisor(3, 1);
	glVertexAttribIPointer(4, 1, GL_INT, sizeof(***instances), 
			(void *) offsetof(struct instance, tile));
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(4, 1);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);
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
	while (!glfwWindowShouldClose(wnd)) {
		glfwPollEvents();
		t1 = glfwGetTime();
		dt = t1 - t0;
		t0 = t1;
		printf("\r%f", 1.0F / dt);
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
		glDrawElementsInstanced(GL_TRIANGLES, COUNT_OF(indices), 
				GL_UNSIGNED_BYTE, NULL, CHUNK * CHUNK * CHUNK); 
		glfwSwapBuffers(wnd);
	}
	return EXIT_SUCCESS;
}
