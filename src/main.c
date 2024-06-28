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

#define CHUNK_LEN 16 
#define MAX_BLOCKS (CHUNK_LEN * CHUNK_LEN * CHUNK_LEN)
#define MAX_QUEUE (MAX_BLOCKS)
#define MAX_VERTICES (36 * MAX_BLOCKS) 

#define NEG_X 0
#define POS_X 1
#define NEG_Y 2
#define POS_Y 3
#define NEG_Z 4
#define POS_Z 5

#define LEFT     1U
#define RIGHT    2U
#define BOTTOM   4U
#define TOP      8U
#define BACK    16U
#define FRONT   32U

#define VAO_MAP       0
#define VAO_CROSSHAIR 1
#define NUM_VAOS      2

#define VBO_MAP       0
#define VBO_CROSSHAIR 1
#define EBO_CROSSHAIR 2
#define NUM_BOS       3 

#define FOR_XYZ_ALL \
	for (x = 0; x < CHUNK_LEN; x++) \
		for (y = 0; y < CHUNK_LEN; y++) \
			for (z = 0; z < CHUNK_LEN; z++) 

#define FOR_XYZ(pos, min, max) \
	for (pos[0] = min[0]; pos[0] < max[0]; pos[0]++) \
		for (pos[1] = min[1]; pos[1] < max[1]; pos[1]++) \
			for (pos[2] = min[2]; pos[2] < max[2]; pos[2]++) 

#define IV3_IDX(a, v) (a[(v)[0]][(v)[1]][(v)[2]])

struct vertex {
	uint32_t x : 5;
	uint32_t y : 5;
	uint32_t z : 5;
	uint32_t u : 4;
	uint32_t v : 4;
	uint32_t l : 8;
};

struct prism {
	vec3 min;
	vec3 max;
};

struct light_node {
	ivec3 v;
	int l;
};

static struct vertex cube[6][6] = {
	{
		{0, 1, 1, 0, 0},
		{0, 1, 0, 1, 0},
		{0, 0, 0, 1, 1},
		{0, 0, 0, 1, 1},
		{0, 0, 1, 0, 1},
		{0, 1, 1, 0, 0},
	},
	{
		{1, 1, 1, 1, 0},
		{1, 0, 1, 1, 1},
		{1, 0, 0, 0, 1},
		{1, 0, 0, 0, 1},
		{1, 1, 0, 0, 0},
		{1, 1, 1, 1, 0},
	},
	{
		{1, 0, 1, 0, 0},
		{0, 0, 1, 1, 0},
		{0, 0, 0, 1, 1},
		{0, 0, 0, 1, 1},
		{1, 0, 0, 0, 1},
		{1, 0, 1, 0, 0},
	},
	{
		{1, 1, 1, 0, 1},
		{1, 1, 0, 0, 0},
		{0, 1, 0, 1, 0},
		{0, 1, 0, 1, 0},
		{0, 1, 1, 1, 1},
		{1, 1, 1, 0, 1},
	},
	{
		{1, 1, 0, 1, 0},
		{1, 0, 0, 1, 1},
		{0, 0, 0, 0, 1},
		{0, 0, 0, 0, 1},
		{0, 1, 0, 0, 0},
		{1, 1, 0, 1, 0},
	},
	{
		{1, 1, 1, 0, 0},
		{0, 1, 1, 1, 0},
		{0, 0, 1, 1, 1},
		{0, 0, 1, 1, 1},
		{1, 0, 1, 0, 1},
		{1, 1, 1, 0, 0},
	},
};

static uint8_t crosshair_indices[] = {
	0, 1, 2,
	0, 2, 3,
	4, 5, 6,
	4, 6, 7,
};

static vec2 crosshair_vertices[] = {
	{0.05F, 0.005F},
	{-0.05F, 0.005F},
	{-0.05F, -0.005F},
	{0.05F, -0.005F},
	{0.005F, 0.05F},
	{-0.005F, 0.05F},
	{-0.005F, -0.05F},
	{0.005F, -0.05F},
};

static struct prism model_prism = { 
	{-0.3F, 0.0F, -0.3F},
	{0.3F, 1.8F, 0.3F} 
};

#define AIR   0
#define DIRT  1
#define GRASS 2

static uint8_t blocks[][6] = {
	{},
	{0, 0, 0, 0, 0, 0},
	{2, 2, 0, 1, 2, 2},
};

static uint8_t light_factors[6] = {
	8, 8, 5, 10, 6, 6
};

static vec3 player_pos;
static vec3 player_vel;
static int grounded;

static uint8_t map[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
static uint8_t faces[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
static uint8_t lights[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN][6];
static struct vertex vertices[MAX_VERTICES];
static int nvertices;

static int width = 640;
static int height = 480;

static vec3 eye; 
static vec3 front = {0.0F, 0.0F, -1.0F};
static vec3 forw = {0.0F, 0.0F, -1.0F};
static vec3 up = {0.0F, 1.0F, 0.0F};
static vec3 right;

static float dt;

static ivec3 pos_itc;
static uint8_t *block_itc;
static int axis_itc;

static struct light_node queue[MAX_QUEUE];
static int head;
static int tail;
static int nqueue;

static float yaw; 
static float pitch;

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

static unsigned load_shader(GLenum type, const char *path) {
	unsigned shader;
	int status;
	char log[1024];
	const char *src;
	char full[PATH_MAX];

	snprintf(full, PATH_MAX, "shader/%s", path);
	src = read_all_str(full);
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

static unsigned load_prog(const char *vs_path, const char *fs_path) {
	unsigned prog;
	int status;
	char log[1024];
	unsigned vs, fs;

	vs = load_shader(GL_VERTEX_SHADER, vs_path);
	fs = load_shader(GL_FRAGMENT_SHADER, fs_path);
	prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glDetachShader(prog, vs);
	glDetachShader(prog, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (!status) {
		glGetProgramInfoLog(prog, sizeof(log), NULL, log);
		die("load_prog: %s\n", log);
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
}

static void update_cam_dirs(void) {
	front[0] = cosf(yaw) * cosf(pitch);
	front[1] = sinf(pitch);
	front[2] = sinf(yaw) * cosf(pitch);
	forw[0] = cosf(yaw);
	forw[1] = 0.0F;
	forw[2] = sinf(yaw);
	glm_cross(forw, up, right);
}

static void add_vertex(int x, int y, int z) {
	struct vertex c, *v;
	int i, j;
	uint8_t *b;

	b = &map[x][y][z];
	for (i = 0; i < 6; i++) {
		if ((faces[x][y][z] >> i) & 1) {
			for (j = 0; j < 6; j++) {
				v = &vertices[nvertices++];
				c = cube[i][j];
				v->x = x + c.x;
				v->y = y + c.y;
				v->z = z + c.z;
				v->u = blocks[*b][i] + c.u; 
				v->v = c.v;
				v->l = lights[x][y][z][i] *
				       (light_factors[i] + 
				       (b == block_itc) * 2); 
			}
		}
	}
}

static void create_map(void) {
	int x, z;

	for (x = 0; x < CHUNK_LEN; x++) {
		for (z = 0; z < CHUNK_LEN; z++) {
			map[x][0][z] = DIRT;
			map[x][1][z] = GRASS;
		}
	}
}

static int inbounds(ivec3 v) {
	int i;
	
	for (i = 0; i < 3; i++) {
		if (v[i] < 0 || v[i] >= CHUNK_LEN) {
			return 0;
		}
	}
	return 1;
}

static uint8_t *map_get(ivec3 v) {
	return inbounds(v) ? &map[v[0]][v[1]][v[2]] : NULL;
}

static void enqueue(struct light_node *old) {
	struct light_node *new;
	int face;
	uint8_t *light;

	for (face = 0; face < 6; face++) {
		if (nqueue == MAX_QUEUE) {
			die("queue oom\n");
		}
		new = queue + tail;
		*new = *old;
		new->v[face / 2] += face % 2 ? -1 : 1;
		if (!inbounds(new->v)) {
			continue;
		}
		light = &IV3_IDX(lights, new->v)[face];
		if (*light >= new->l) {
			continue;
		}
		*light = new->l;
		if (IV3_IDX(map, new->v)) {
			continue;
		}
		if (face != POS_Y) {
			new->l--;
		}
		if (new->l == 0) {
			continue;
		}
		tail = (tail + 1) % MAX_QUEUE;
		nqueue++;
	}
}

static void dequeue(struct light_node *n) {
	*n = queue[head];
	head = (head + 1) % MAX_QUEUE;
	nqueue--;
}

static void flood(void) {
	int x, z;
	struct light_node q;

	memset(lights, 0, sizeof(lights));
	nqueue = 0;
	for (x = 0; x < CHUNK_LEN; x++) {
		for (z = 0; z < CHUNK_LEN; z++) {
			q.v[0] = x;
			q.v[1] = CHUNK_LEN;
			q.v[2] = z;
			q.l = 15;
			enqueue(&q);
		}
	}
	while (nqueue) {
		dequeue(&q);
		enqueue(&q);
	}
}

static void create_vertices(void) {
	int x, y, z;

	FOR_XYZ_ALL {
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

	flood();

	nvertices = 0;
	FOR_XYZ_ALL {
		if (map[x][y][z]) {
			add_vertex(x, y, z);
		}
	}
}

static int axis_collide(struct prism *a, struct prism *b, int axis) {
	return a->max[axis] > b->min[axis] && b->max[axis] > a->min[axis];
}

static int prism_collide(struct prism *a, struct prism *b) {
	int i;
	for (i = 0; i < 3; i++) {
		if (!axis_collide(a, b, i)) {
			return 0;
		}
	}
	return 1;
}

static int clampi(int v, int l, int h) {
	return v < l ? l : (v > h ? h : v);
}

static void get_player_prism(struct prism *prism) {
	prism->min[0] = player_pos[0] - 0.3F;
	prism->min[1] = player_pos[1];
	prism->min[2] = player_pos[2] - 0.3F; 
	prism->max[0] = player_pos[0] + 0.3F;
	prism->max[1] = player_pos[1] + 1.8F;
	prism->max[2] = player_pos[2] + 0.3F;
}

static void get_moving_player_prism(struct prism *prism, int axis) {
	get_player_prism(prism);
	if (player_vel[axis] < 0.0F) {
		prism->max[axis] = prism->min[axis];
		prism->min[axis] += player_vel[axis] * dt;
	} else {
		prism->min[axis] = prism->max[axis];
		prism->max[axis] += player_vel[axis] * dt;
	}
}

static void get_block_prism(struct prism *prism, ivec3 v) {
	int i;

	for (i = 0; i < 3; i++) {
		prism->min[i] = v[i] - 0.5F;
		prism->max[i] = v[i] + 0.5F;
	}
}

static void get_block_coord(vec3 src, ivec3 dst) {
	int i;

	for (i = 0; i < 3; i++) {
		dst[i] = floorf(src[i] + 0.5F);
	}
}

static void get_detect_bounds(struct prism *prism, ivec3 min, ivec3 max) {
	int i;

	for (i = 0; i < 3; i++) {
		min[i] = floorf(prism->min[i] - 0.5F);
		max[i] = ceilf(prism->max[i] + 0.5F);
		min[i] = clampi(min[i], 0, CHUNK_LEN);
		max[i] = clampi(max[i], 0, CHUNK_LEN);
	}
}

static void axis_move(int axis) {
	struct prism player_prism;
	struct prism block_prism;
	ivec3 pos, min, max;
	float new, tmp;
	float vel;

	if (fabsf(player_vel[axis]) < 1e-5F) {
		return;
	}
	vel = player_vel[axis];
	get_moving_player_prism(&player_prism, axis);
	get_detect_bounds(&player_prism, min, max);
	new = player_pos[axis] + vel * dt;
	FOR_XYZ (pos, min, max) {
		if (!map[pos[0]][pos[1]][pos[2]]) {
			continue;
		}
		get_block_prism(&block_prism, pos);
		if (!prism_collide(&player_prism, &block_prism)) {
			continue;
		}
		if (vel < 0.0F) {
			tmp = block_prism.max[axis] - model_prism.min[axis];
			new = fmaxf(new, tmp);
			if (axis == 1) {
				grounded = 1;
			}
		} else {
			tmp = block_prism.min[axis] - model_prism.max[axis];
			new = fminf(new, tmp);
		}
		player_vel[axis] = 0.0F;
	}
	player_pos[axis] = new;
}

static void player_move(void) {
	int axis;

	for (axis = 0; axis < 3; axis++) {
		axis_move(axis);
	}
}

static int vec3_min_idx(vec3 v) {
	int i, min;

	min = 0;
	for (i = 1; i < 3; i++) {
		if (v[i] < v[min]) {
			min = i;
		}
	}
	return min;
}

static void find_block_itc(void) {
	vec3 delta;
	ivec3 step;
	vec3 disp;
	int i;
	vec3 v;

	get_block_coord(eye, pos_itc);
	for (i = 0; i < 3; i++) {
		delta[i] = fabsf(1.0F / front[i]);
		if (front[i] < 0.0F) {
			step[i] = -1;
			disp[i] = eye[i] - pos_itc[i];
		} else {
			step[i] = 1;
			disp[i] = pos_itc[i] - eye[i];
		}
		disp[i] = (disp[i] + 0.5F) * delta[i];
	}
	for (i = 0; i < 9; i++) {
		axis_itc = vec3_min_idx(disp);
		disp[axis_itc] += delta[axis_itc]; 
		pos_itc[axis_itc] += step[axis_itc];
		block_itc = map_get(pos_itc);
		if (!block_itc) {
			return;
		}
		if (*block_itc) {
			break;
		}
	} 
	for (i = 0; i < 3; i++) {
		v[i] = pos_itc[i];
	}
	if (glm_vec3_distance2(v, eye) > 25.0F) {
		block_itc = NULL;
	}
}

static void remove_block(void) {
	if (block_itc) {
		*block_itc = 0;
	}
}

static void add_block(void) {
	ivec3 place_pos;
	struct prism player_prism;
	struct prism block_prism;
	uint8_t *block;

	if (!block_itc) {
		return;
	}
	glm_ivec3_copy(pos_itc, place_pos);
	if (front[axis_itc] < 0.0F) {
		place_pos[axis_itc]++; 
	} else {
		place_pos[axis_itc]--; 
	}
	get_player_prism(&player_prism);
	get_block_prism(&block_prism, place_pos);
	if (prism_collide(&player_prism, &block_prism)) {
		return;
	}
	block = map_get(place_pos);
	if (block) {
		*block = 1;
	}
}

int main(void) {
	GLFWwindow *wnd;
	unsigned block_prog, crosshair_prog;
	unsigned vaos[NUM_VAOS], bos[NUM_BOS];
	int world_loc;
	int tex_loc;
	int proj_loc;
	mat4 proj, view, world;
	stbi_uc *data;
	unsigned tex;
	int w, h, channels;
	double t0, t1;
	vec3 center;
	int held_left, held_right;
	vec3 aspect;

	if (!glfwInit()) {
		glfw_die("glfwInit");
	}
	atexit(glfwTerminate);
	create_map();
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	data = NULL;
	glGenVertexArrays(NUM_VAOS, vaos);
	glGenBuffers(NUM_BOS, bos);
	glBindVertexArray(vaos[VAO_MAP]);
	glBindBuffer(GL_ARRAY_BUFFER, bos[VBO_MAP]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), 
			NULL, GL_DYNAMIC_DRAW);
	glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 4, NULL);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	glBindVertexArray(vaos[VAO_CROSSHAIR]);
	glBindBuffer(GL_ARRAY_BUFFER, bos[VBO_CROSSHAIR]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_vertices), 
			crosshair_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, NULL);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bos[EBO_CROSSHAIR]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(crosshair_indices),
			crosshair_indices, GL_STATIC_DRAW);
	block_prog = load_prog("map.vert", "map.frag");
	crosshair_prog = load_prog("crosshair.vert", "crosshair.frag");
	world_loc = glGetUniformLocation(block_prog, "world");
	tex_loc = glGetUniformLocation(block_prog, "tex");
	proj_loc = glGetUniformLocation(crosshair_prog, "proj");
	glfwSetFramebufferSizeCallback(wnd, resize_cb);
	glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(wnd, mouse_cb);
	glfwSwapInterval(1);
	glfwShowWindow(wnd);
	t0 = glfwGetTime();
	glUseProgram(block_prog);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(tex_loc, 0);
	held_left = 0;
	held_right = 0;
	while (!glfwWindowShouldClose(wnd)) {
		glfwPollEvents();
		t1 = glfwGetTime();
		dt = clamp(t1 - t0, 0.0001F, 1000.0F);
		t0 = t1;
		update_cam_dirs();
		player_vel[0] = 0.0F;
		player_vel[2] = 0.0F;
		if (glfwGetKey(wnd, GLFW_KEY_W)) {
			glm_vec3_muladds(forw, 5.0F, player_vel);
		} 
		if (glfwGetKey(wnd, GLFW_KEY_S)) {
			glm_vec3_mulsubs(forw, 5.0F, player_vel);
		}
		if (glfwGetKey(wnd, GLFW_KEY_A)) {
			glm_vec3_mulsubs(right, 5.0F, player_vel);
		} 
		if (glfwGetKey(wnd, GLFW_KEY_D)) {
			glm_vec3_muladds(right, 5.0F, player_vel);
		}
		if (glfwGetKey(wnd, GLFW_KEY_SPACE) && grounded) {
			glm_vec3_muladds(up, 8.0F, player_vel);
		}
		grounded = 0;
		player_move();
		player_vel[1] = fmaxf(player_vel[1] - 24.0F * dt, -64.0F);
		glm_vec3_copy(player_pos, eye);
		eye[1] += 1.7F;
		find_block_itc();
		if (glfwGetMouseButton(wnd, GLFW_MOUSE_BUTTON_LEFT) 
				== GLFW_PRESS) {
			if (!held_left) {
				add_block();
				held_left = 1;
			}
		} else {
			held_left = 0;
		}
		if (glfwGetMouseButton(wnd, GLFW_MOUSE_BUTTON_RIGHT) 
				== GLFW_PRESS) {
			if (!held_right) {
				remove_block();
				held_right = 1;
			}
		} else {
			held_right = 0;
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.5F, 0.6F, 1.0F, 1.0F);
		glm_perspective(GLM_PI_4f, width / (float) height, 
				0.01F, 100.0F, proj); 
		glm_vec3_add(eye, front, center);
		glm_lookat(eye, center, up, view);
		glm_mat4_mul(proj, view, world);
		create_vertices();
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LESS);
		glCullFace(GL_BACK);
		glUseProgram(block_prog);
		glUniformMatrix4fv(world_loc, 1, GL_FALSE, (float *) world);
		glBindVertexArray(vaos[VAO_MAP]);
		glBindBuffer(GL_ARRAY_BUFFER, bos[VBO_MAP]);
		glBufferSubData(GL_ARRAY_BUFFER, 0,
				nvertices * sizeof(*vertices), 
				vertices);
		glDrawArrays(GL_TRIANGLES, 0, nvertices);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glUseProgram(crosshair_prog);
		glBindVertexArray(vaos[VAO_CROSSHAIR]);
		aspect[0] = (float) height / width;
		aspect[1] = 1.0F;
		aspect[2] = 1.0F;
		glm_scale_make(proj, aspect);
		glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *) proj);
		glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_BYTE, NULL); 
		glfwSwapBuffers(wnd);
	}
	return EXIT_SUCCESS;
}
