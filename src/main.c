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

#define COUNT_OF(ary) (sizeof(ary) / sizeof(*(ary)))
#define END_OF(ary) ((ary) + COUNT_OF((ary)))

struct vertex {
	vec3 pos;
	vec3 nor;
	vec2 tex;
};

static GLushort indices[] = {
	0, 1, 2,
	2, 3, 0,
	4, 5, 6,
	6, 7, 4,
	8, 9, 10,
	10, 11, 8,
	12, 13, 14,
	14, 15, 12,
	16, 17, 18,
	18, 19, 16,
	20, 21, 22,
	22, 23, 20
};

static struct vertex vertices[] = {
	{{-0.5F, -0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}},
	{{0.5F, -0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}},
	{{0.5F, 0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}},
	{{-0.5F, 0.5F, -0.5F}, {0.0F, 0.0F, -1.0F}},
	{{-0.5F, -0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}},
	{{0.5F, -0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}},
	{{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}},
	{{-0.5F, 0.5F, 0.5F}, {0.0F, 0.0F, 1.0F}},
	{{-0.5F, 0.5F, 0.5F}, {-1.0F, 0.0F, 0.0F}},
	{{-0.5F, 0.5F, -0.5F}, {-1.0F, 0.0F, 0.0F}},
	{{-0.5F, -0.5F, -0.5F}, {-1.0F, 0.0F, 0.0F}},
	{{-0.5F, -0.5F, 0.5F}, {-1.0F, 0.0F, 0.0F}},
	{{0.5F, 0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}},
	{{0.5F, 0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}},
	{{0.5F, -0.5F, -0.5F}, {1.0F, 0.0F, 0.0F}},
	{{0.5F, -0.5F, 0.5F}, {1.0F, 0.0F, 0.0F}},
	{{-0.5F, -0.5F, -0.5F}, {0.0F, -1.0F, 0.0F}},
	{{0.5F, -0.5F, -0.5F}, {0.0F, -1.0F, 0.0F}},
	{{0.5F, -0.5F, 0.5F}, {0.0F, -1.0F, 0.0F}},
	{{-0.5F, -0.5F, 0.5F}, {0.0F, -1.0F, 0.0F}},
	{{-0.5F, 0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}},
	{{0.5F, 0.5F, -0.5F}, {0.0F, 1.0F, 0.0F}},
	{{0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}},
	{{-0.5F, 0.5F, 0.5F}, {0.0F, 1.0F, 0.0F}}
};

static int width = 640;
static int height = 480;

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

static void normal_model(mat4 src, mat3 dst) {
	mat4 tmp;
	glm_mat4_inv(src, tmp);
	glm_mat4_pick3t(tmp, dst);
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
	char *slash, *end, *nul;

	if (!realpath("/proc/self/exe", path)) {
		die("realpath(): %s\n", strerror(errno));
	}
	slash = strrchr(path, '/');
	if (!slash) {
		die("path missing slash\n");
	}
	end = END_OF(path);
	nul = stpecpy(slash + 1, end, "res");
	if (nul == end) {
		die("path too long\n");
	}
	if (chdir(path) < 0) {
		die("chdir(): %s\n", strerror(errno));
	}
}

int main(void) {
	GLFWwindow *wnd;
	GLuint vs, fs;
	GLuint prog;
	GLuint vao, bos[2];
	GLint proj_loc;
	GLint view_loc;
	GLint model_loc;
	GLint tex_loc;
	mat4 proj, view, model;
	mat3 invt;
	stbi_uc *data;
	GLuint tex;
	int w, h, channels;

	set_data_path();
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
	data = stbi_load("img/atlas.png", &w, &h, &channels, 3);
	if (!data) {
		die("stbi_load: %s\n", stbi_failure_reason());
	}
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
			GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
			GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 
			0, GL_RGB, GL_UNSIGNED_BYTE, data); 
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	data = NULL;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, bos);
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
	glBindVertexArray(0);
	vs = load_shader(GL_VERTEX_SHADER, "shader/vert.glsl"); 
	fs = load_shader(GL_FRAGMENT_SHADER, "shader/frag.glsl");
	prog = load_prog(vs, fs);
	proj_loc = glGetUniformLocation(prog, "proj");
	view_loc = glGetUniformLocation(prog, "view");
	model_loc = glGetUniformLocation(prog, "model");
	tex_loc = glGetUniformLocation(prog, "tex");
	glfwSetFramebufferSizeCallback(wnd, resize_cb);
	glfwSwapInterval(1);
	glfwShowWindow(wnd);
	while (!glfwWindowShouldClose(wnd)) {
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);
		glm_perspective(GLM_PI_4f, width / (float) height, 
				0.1F, 100.0F, proj); 
		glm_mat4_identity(view);
		glm_translate_z(view, -3.0F);
		glm_mat4_identity(model);
		normal_model(model, invt);
		static float rot;
		glm_rotate_x(model, rot, model);
		rot += 0.01F;
		glUseProgram(prog);
		glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *) proj);
		glUniformMatrix4fv(view_loc, 1, GL_FALSE, (float *) view);
		glUniformMatrix4fv(model_loc, 1, GL_FALSE, (float *) model);
		glUniform1i(tex_loc, 0);
		glBindVertexArray(vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glDrawElements(GL_TRIANGLES, COUNT_OF(indices), 
				GL_UNSIGNED_SHORT, NULL); 
		glfwSwapBuffers(wnd);
	}
	return EXIT_SUCCESS;
}
