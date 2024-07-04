#include <cglm/cglm.h>
#include <cglm/struct.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stb_image.h>
#include <math.h>

#define MAX_PITCH (GLM_PI_2f - 0.01F)
#define MIN_PITCH (-MAX_PITCH)
#define SENSITIVITY 0.1F

#define CHUNK_LEN 32 
#define MAX_BLOCKS (CHUNK_LEN * CHUNK_LEN * CHUNK_LEN)
#define MAX_QUEUE (MAX_BLOCKS)
#define MAX_VERTICES (24 * MAX_BLOCKS) 
#define MAX_INDICES (36 * MAX_BLOCKS) 

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
#define EBO_MAP       1
#define VBO_CROSSHAIR 2
#define EBO_CROSSHAIR 3
#define NUM_BOS       4 

#define OBJ_PLAYER 0
#define OBJ_ITEM   1

#define GROUNDED 1U
#define STUCK    2U

#define FOR_XYZ_ALL \
	for (x = 0; x < CHUNK_LEN; x++) \
		for (y = 0; y < CHUNK_LEN; y++) \
			for (z = 0; z < CHUNK_LEN; z++) 

#define FOR_XYZ(pos, min, max) \
	for (pos[0] = min[0]; pos[0] < max[0]; pos[0]++) \
		for (pos[1] = min[1]; pos[1] < max[1]; pos[1]++) \
			for (pos[2] = min[2]; pos[2] < max[2]; pos[2]++) 

#define IV3_IDX(a, v) (a[(v)[0]][(v)[1]][(v)[2]])

#define ENABLE_ATTRIB(i, n, type, member) enable_attrib(i, n, \
		sizeof(type), offsetof(type, member))

struct vertex {
	vec3 xyz; 
	vec2 uv;
	float lum;
};

struct prism {
	vec3 min;
	vec3 max;
};

struct light_node {
	ivec3 v;
	int l;
};

struct hotbar_slot {
	uint8_t id;
	uint8_t n; 
};

struct object {
	int type;
	unsigned flags;
	vec3 pos;
	vec3 vel;
	int id;
	float rot;
};

struct prism locals[] = {
	[OBJ_PLAYER] = {
		{-0.3F, 0.0F, -0.3F},
		{0.3F, 1.8F, 0.3F} 
	},
	[OBJ_ITEM] = {
		{-0.25F, -0.25F, -0.25F},
		{0.25F, 0.25F, 0.25F},
	}
};

struct object player = {
	.type =  OBJ_PLAYER,
	.pos = {8.0F, 1.5F, 8.0F}
};

static uint32_t cube_indices[] = {
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
	22, 23, 20,
};

static struct vertex cube_vertices[24] = {
	{{-0.5F, 0.5F, 0.5F}, {1.0F, 0.0F}},
	{{-0.5F, 0.5F, -0.5F}, {0.0F, 0.0F}},
	{{-0.5F, -0.5F, -0.5F}, {0.0F, 1.0F}},
	{{-0.5F, -0.5F, 0.5F}, {1.0F, 1.0F}},
	{{0.5F, 0.5F, 0.5F}, {0.0F, 0.0F}},
	{{0.5F, -0.5F, 0.5F}, {0.0F, 1.0F}},
	{{0.5F, -0.5F, -0.5F}, {1.0F, 1.0F}},
	{{0.5F, 0.5F, -0.5F}, {1.0F, 0.0F}},
	{{0.5F, -0.5F, 0.5F}, {1.0F, 0.0F}},
	{{-0.5F, -0.5F, 0.5F}, {0.0F, 0.0F}},
	{{-0.5F, -0.5F, -0.5F}, {0.0F, 1.0F}},
	{{0.5F, -0.5F, -0.5F}, {1.0F, 1.0F}},
	{{0.5F, 0.5F, 0.5F}, {1.0F, 1.0F}},
	{{0.5F, 0.5F, -0.5F}, {1.0F, 0.0F}},
	{{-0.5F, 0.5F, -0.5F}, {0.0F, 0.0F}},
	{{-0.5F, 0.5F, 0.5F}, {0.0F, 1.0F}},
	{{0.5F, 0.5F, -0.5F}, {0.0F, 0.0F}},
	{{0.5F, -0.5F, -0.5F}, {0.0F, 1.0F}},
	{{-0.5F, -0.5F, -0.5F}, {1.0F, 1.0F}},
	{{-0.5F, 0.5F, -0.5F}, {1.0F, 0.0F}},
	{{0.5F, 0.5F, 0.5F}, {1.0F, 0.0F}},
	{{-0.5F, 0.5F, 0.5F}, {0.0F, 0.0F}},
	{{-0.5F, -0.5F, 0.5F}, {0.0F, 1.0F}},
	{{0.5F, -0.5F, 0.5F}, {1.0F, 1.0F}},
};


static struct vertex square[] = {
	{{0.5F, 0.5F}, {1.0F, 0.0F}},
	{{-0.5F, 0.5F}, {0.0F, 0.0F}},
	{{-0.5F, -0.5F}, {0.0F, 1.0F}},
	{{-0.5F, -0.5F}, {0.0F, 1.0F}},
	{{0.5F, -0.5F}, {1.0F, 1.0F}},
	{{0.5F, 0.5F}, {1.0F, 0.0F}},
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

#define AIR   0
#define DIRT  1
#define GRASS 2

static vec2 block_uvs[][6] = {
	[AIR] = {},
	[DIRT] = {
		{0.0F, 0.0F}, 
		{0.0F, 0.0F}, 
		{0.0F, 0.0F}, 
		{0.0F, 0.0F}, 
		{0.0F, 0.0F}, 
		{0.0F, 0.0F},
	},
	[GRASS] = {
		{2.0F, 0.0F},
		{2.0F, 0.0F},
		{0.0F, 0.0F},
		{1.0F, 0.0F},
		{2.0F, 0.0F},
		{2.0F, 0.0F},
	}
};

static float light_factors[6] = {
	0.9F, 0.9F, 0.7F, 1.0F, 0.8F, 0.8F
};

static uint8_t map[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
static uint8_t faces[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN];
static uint8_t lights[CHUNK_LEN][CHUNK_LEN][CHUNK_LEN][6];

static struct vertex vertices[MAX_VERTICES];
static uint32_t indices[MAX_INDICES];
static int nvertices;
static int nindices;

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

static struct hotbar_slot hotbar_slots[10];
static int hotbar_sel;

struct object items[MAX_BLOCKS];
static int num_items;

static void ivec3_to_vec3(ivec3 iv, vec3 v) {
	v[0] = iv[0];
	v[1] = iv[1];
	v[2] = iv[2];
}

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
	char *full;

	if (asprintf(&full, "shader/%s", path) < 0) {
		die("asprintf oom\n");
	}
	src = read_all_str(full);
	free(full);
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

static void set_data_path(void) {
	char *path, *nul;
	size_t n;

	path = realpath("/proc/self/exe", NULL);
	if (!path) {
		die("realpath(): %s\n", strerror(errno));
	}
	nul = strrchr(path, '/');
	if (!nul) {
		die("path missing slash\n");
	}
	n = nul + 1 - path;
	path = realloc(path, n + sizeof("res") + 1);
	if (!path) {
		die("path oom\n");
	}
	strcpy(path + n, "res");
	if (chdir(path) < 0) {
		die("chdir(): %s\n", strerror(errno));
	}
	free(path);
}

static float clampf(float v, float l, float h) {
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
	pitch = clampf(pitch, MIN_PITCH, MAX_PITCH);
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

static void add_faces(int x, int y, int z) {
	struct vertex *v;
	int i, j;
	uint8_t *b;
	uint32_t *idx;

	b = &map[x][y][z];
	for (i = 0; i < 6; i++) {
		if ((faces[x][y][z] >> i) & 1) {
			for (j = 0; j < 6; j++) {
				idx = indices + nindices++;
				*idx = nvertices + cube_indices[j];
			}
			for (j = 0; j < 4; j++) {
				v = &vertices[nvertices++];
				*v = cube_vertices[i * 4 + j];
				v->xyz[0] += x;
				v->xyz[1] += y;
				v->xyz[2] += z;
				glm_vec2_add(v->uv, block_uvs[*b][i], v->uv); 
				glm_vec2_divs(v->uv, 16.0F, v->uv);
				v->lum = (lights[x][y][z][i] + 1) / 16.0F * 
					 light_factors[i]; 
				if (b == block_itc) {
					v->lum *= 1.2F;
				}
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

static void create_item_vertices(void) {
	int i, j;
	struct vertex *v;
	float c, s, x, z;
	vec2 *uv;

	for (i = 0; i < num_items; i++) {
		for (j = 0; j < 36; j++) {
			indices[nindices++] = cube_indices[j] + nvertices;
		}
		c = cosf(items[i].rot);
		s = sinf(items[i].rot);
		uv = block_uvs[items[i].id];
		for (j = 0; j < 24; j++) {
			v = vertices + nvertices++;
			*v = cube_vertices[j];
			x = v->xyz[0];
			z = v->xyz[2];
			v->xyz[0] = x * c - z * s; 
			v->xyz[2] = x * s + z * c;
			glm_vec3_divs(v->xyz, 4.0F, v->xyz);
			glm_vec3_add(v->xyz, items[i].pos, v->xyz);
			glm_vec2_add(v->uv, uv[j / 4], v->uv);
			glm_vec2_divs(v->uv, 16.0F, v->uv);
			v->lum = light_factors[j / 4];
		}
		items[i].rot += dt;
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
	nindices = 0;
	FOR_XYZ_ALL {
		if (map[x][y][z]) {
			add_faces(x, y, z);
		}
	}
	create_item_vertices();
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

static void get_world_prism(struct prism *world, struct object *obj) {
	struct prism *local;
	
	local = locals + obj->type;
	glm_vec3_add(obj->pos, local->min, world->min);
	glm_vec3_add(obj->pos, local->max, world->max);
}

static void get_disp_prism(struct object *obj, struct prism *disp, int axis) {
	get_world_prism(disp, obj);
	if (obj->vel[axis] < 0.0F) {
		disp->max[axis] = disp->min[axis];
		disp->min[axis] += obj->vel[axis] * dt;
	} else {
		disp->min[axis] = disp->max[axis];
		disp->max[axis] += obj->vel[axis] * dt;
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
		dst[i] = roundf(src[i]);
	}
}

static int clampi(int v, int l, int h) {
	return v < l ? l : (v > h ? h : v);
}

static void get_detect_bounds(struct prism *prism, ivec3 min, ivec3 max) {
	int i;

	for (i = 0; i < 3; i++) {
		min[i] = floorf(prism->min[i] + 0.5F);
		max[i] = ceilf(prism->max[i] + 0.5F);
		min[i] = clampi(min[i], 0, CHUNK_LEN);
		max[i] = clampi(max[i], 0, CHUNK_LEN);
	}
}

static int add_to_hotbar(int id) {
	int i;
	struct hotbar_slot *slot, *empty;

	empty = NULL;
	for (i = 0; i < 10; i++) {
		slot = hotbar_slots + i;
		if (slot->id == id && 
		    slot->n > 0 && slot->n < 64) {
			slot->n++;
			return 1;
		}
		if (!empty && !slot->n) {
			empty = slot;
		}
	}
	if (!empty) {
		return 0;
	}	
	empty->id = id;
	empty->n = 1;
	return 1;
}

static void player_item_col(struct prism *disp) {
	struct prism block; 
	int i;

	for (i = 0; i < num_items; ) {
		get_world_prism(&block, items + i);
		if (prism_collide(disp, &block) &&
		    add_to_hotbar(items[i].id)) {
			items[i] = items[--num_items];
		} else {
			i++;
		}
	}
}

static void move(struct object *obj) {
	struct prism disp, block;
	struct prism *local;
	ivec3 pos, min, max;
	float *blockf, *dispf, *posf, *velf;
	float (*sel)(float, float);
	float localf, off;
	int axis, cols;

	local = locals + obj->type;
	if (obj->flags & STUCK) {
		for (axis = 0; axis < 3; axis++) {
			velf = obj->vel + axis;
			if (*velf == 0.0F) {
				continue;
			}
			get_disp_prism(obj, &disp, axis);
			get_detect_bounds(&disp, min, max);
			posf = obj->pos + axis;
			if (*velf < 0.0F) {
				dispf = disp.min + axis;
				localf = local->min[axis];
				off = 0.5F + localf * 2;
				blockf = block.max + axis;
				sel = fmaxf;
			} else {
				dispf = disp.max + axis;
				localf = local->max[axis];
				off = -0.5F + localf * 2;
				blockf = block.min + axis;
				sel = fminf;
			}
			cols = 0;
			FOR_XYZ (pos, min, max) {
				if (IV3_IDX(map, pos)) {
					continue;
				}
				get_block_prism(&block, pos);
				*blockf = pos[axis] + off;
				if (prism_collide(&disp, &block)) {
					*dispf = sel(*blockf, *dispf);
					cols = 1;
				}
			}
			if (cols) {
				*velf = 0.0F;
				obj->flags &= ~STUCK;
			}
			*posf = *dispf - localf;
		}
	}
	if (!(obj->flags & STUCK)) {
		for (axis = 0; axis < 3; axis++) {
			velf = obj->vel + axis;
			if (*velf == 0.0F) {
				continue;
			}
			get_disp_prism(obj, &disp, axis);
			get_detect_bounds(&disp, min, max);
			posf = obj->pos + axis;
			if (*velf < 0.0F) {
				dispf = disp.min + axis;
				localf = local->min[axis];
				off = 0.5F;
				sel = fmaxf;
			} else {
				dispf = disp.max + axis;
				localf = local->max[axis];
				off = -0.5F;
				sel = fminf;
			}
			cols = 0;
			FOR_XYZ (pos, min, max) {
				if (IV3_IDX(map, pos)) {
					*dispf = sel(pos[axis] + off, *dispf);
					cols = 1;
				}
			}
			if (cols) {
				if (*velf < 0.0F && axis == 1) {
					obj->flags |= GROUNDED;
				}
				*velf = 0.0F;
			}
			if (obj->type == OBJ_PLAYER) {
				player_item_col(&disp);
			}
			*posf = *dispf - localf;
		}
		obj->vel[1] = fmaxf(obj->vel[1] - 24.0F * dt, -64.0F);
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
	ivec3_to_vec3(pos_itc, v);
	if (glm_vec3_distance2(v, eye) > 25.0F) {
		block_itc = NULL;
	}
}

static void remove_block(void) {
	struct object *item;

	if (!block_itc) {
		return;
	}
	if (num_items == MAX_BLOCKS) {
		die("items oom\n");
	}
	item = items + num_items++;
	item->type = OBJ_ITEM;
	item->flags = 0;
	ivec3_to_vec3(pos_itc, item->pos);
	glm_vec3_zero(item->vel); 
	item->id = *block_itc;
	item->rot = 0.0F;
	*block_itc = 0;
}

static void add_square(vec2 uv, float div, vec2 xy, float scale, float lum) {
	int i;
	struct vertex *v;

	for (i = 0; i < 6; i++) { 
		v = vertices + nvertices++;
		glm_vec2_add(square[i].uv, uv, v->uv);
		glm_vec2_divs(v->uv, div, v->uv);
		glm_vec2_scale(square[i].xyz, scale, v->xyz);
		glm_vec2_add(v->xyz, xy, v->xyz);
		v->xyz[2] = 0.0F;
		v->lum = lum;
	}
}

static void render_digit(int val, float x) {
	vec2 uv, xy;

	uv[0] = (int) (val / 4) * 2 + val % 2 + 8.0F;
	uv[1] = val / 2 % 2;
	xy[0] = x;
	xy[1] = -6.85F;
	add_square(uv, 32.0F, xy, 0.3F, 1.0F);
}

static void hotbar_add(void) {
	int i;
	struct hotbar_slot *slot;
	vec2 uv;
	vec2 xy;
	float lum;

	nvertices = 0;
	for (i = 0; i < 10; i++) {
		uv[0] = 3;
		uv[1] = 0;
		xy[0] = i - 5;
		xy[1] = -7;
		lum = (i == hotbar_sel) ? 2.5F : 1.0F;
		add_square(uv, 16.0F, xy, 1.0F, lum);
		slot = hotbar_slots + i;
		if (!slot->n) {
			continue;
		}
		xy[0] = i - 5.0F; 
		xy[1] = -7.0F;
		add_square(block_uvs[slot->id][5], 16.0F, 
				xy, 0.6F, 1.0F);
		if (slot->n > 9) {
			render_digit(slot->n / 10, i - 5.1F);
		}
		render_digit(slot->n % 10, i - 4.8F);
	}
}

static void add_block(void) {
	ivec3 place_pos;
	struct prism obj_prism;
	struct prism block_prism;
	uint8_t *block;
	struct hotbar_slot *slot;
	int i;
	struct object *obj;

	if (!block_itc) {
		return;
	}
	glm_ivec3_copy(pos_itc, place_pos);
	if (front[axis_itc] < 0.0F) {
		place_pos[axis_itc]++; 
	} else {
		place_pos[axis_itc]--; 
	}
	block = map_get(place_pos);
	if (!block) {
		return;
	}
	get_block_prism(&block_prism, place_pos);
	get_world_prism(&obj_prism, &player);
	if (prism_collide(&obj_prism, &block_prism)) {
		return;
	}
	slot = hotbar_slots + hotbar_sel;
	if (!slot->n) {
		return;
	}
	*block = slot->id; 
	slot->n--;
	find_block_itc();
	for (i = 0; i < num_items; i++) {
		obj = items + i;
		get_world_prism(&obj_prism, obj);
		if (prism_collide(&obj_prism, &block_prism)) {
			glm_vec3_zero(obj->vel);
			if (front[axis_itc] < 0.0F) {
				obj->vel[axis_itc] = 3.0F;
			} else {
				obj->vel[axis_itc] = -3.0F;
			}
			obj->flags |= STUCK;
		}
	}
}

static void enable_attrib(int i, int n, size_t stride, size_t offset) {
	glVertexAttribPointer(i, n, GL_FLOAT, 
			GL_FALSE, stride, (void *) offset);
	glEnableVertexAttribArray(i);
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
	int i;

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
	data = stbi_load("img/atlas.png", &w, &h, &channels, 4);
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 
			0, GL_RGBA, GL_UNSIGNED_BYTE, data); 
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
	ENABLE_ATTRIB(0, 3, struct vertex, xyz); 
	ENABLE_ATTRIB(1, 2, struct vertex, uv); 
	ENABLE_ATTRIB(2, 1, struct vertex, lum); 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bos[EBO_MAP]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), 
			NULL, GL_DYNAMIC_DRAW);
	glBindVertexArray(0);
	glBindVertexArray(vaos[VAO_CROSSHAIR]);
	glBindBuffer(GL_ARRAY_BUFFER, bos[VBO_CROSSHAIR]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_vertices), 
			crosshair_vertices, GL_STATIC_DRAW);
	enable_attrib(0, 2, sizeof(vec2), 0);
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
	glDepthFunc(GL_LESS);
	glCullFace(GL_BACK);
	glUseProgram(block_prog);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(tex_loc, 0);
	held_left = 0;
	held_right = 0;
	while (!glfwWindowShouldClose(wnd)) {
		glfwPollEvents();
		t1 = glfwGetTime();
		dt = t1 - t0;
		t0 = t1;
		update_cam_dirs();
		player.vel[0] = 0.0F;
		player.vel[2] = 0.0F;
		if (glfwGetKey(wnd, GLFW_KEY_X)) {
			glm_vec3_muladds(forw, 500.0F, player.vel);
		} 
		if (glfwGetKey(wnd, GLFW_KEY_W)) {
			glm_vec3_muladds(forw, 5.0F, player.vel);
		} 
		if (glfwGetKey(wnd, GLFW_KEY_S)) {
			glm_vec3_mulsubs(forw, 5.0F, player.vel);
		}
		if (glfwGetKey(wnd, GLFW_KEY_A)) {
			glm_vec3_mulsubs(right, 5.0F, player.vel);
		} 
		if (glfwGetKey(wnd, GLFW_KEY_D)) {
			glm_vec3_muladds(right, 5.0F, player.vel);
		}
		if (glfwGetKey(wnd, GLFW_KEY_SPACE) && 
				(player.flags & GROUNDED)) {
			glm_vec3_muladds(up, 8.0F, player.vel);
		}
		for (i = 0; i < 9; i++) {
			if (glfwGetKey(wnd, GLFW_KEY_1 + i)) {
				hotbar_sel = i;
			}
		} 
		if (glfwGetKey(wnd, GLFW_KEY_0)) {
			hotbar_sel = 9;
		}
		player.flags &= ~GROUNDED;
		move(&player);
		glm_vec3_copy(player.pos, eye);
		eye[1] += 1.7F;
		find_block_itc();
		if (glfwGetMouseButton(wnd, GLFW_MOUSE_BUTTON_LEFT) 
				== GLFW_PRESS) {
			if (!held_left) {
				remove_block();
				held_left = 1;
			}
		} else {
			held_left = 0;
		}
		if (glfwGetMouseButton(wnd, GLFW_MOUSE_BUTTON_RIGHT) 
				== GLFW_PRESS) {
			if (!held_right) {
				add_block();
				held_right = 1;
			}
		} else {
			held_right = 0;
		}
		for (i = 0; i < num_items; i++) {
			move(items + i);
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
		glUseProgram(block_prog);
		glUniformMatrix4fv(world_loc, 1, GL_FALSE, (float *) world);
		glBindVertexArray(vaos[VAO_MAP]);
		glBindBuffer(GL_ARRAY_BUFFER, bos[VBO_MAP]);
		glBufferSubData(GL_ARRAY_BUFFER, 0,
				nvertices * sizeof(*vertices), 
				vertices);
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
				nindices * sizeof(*indices),
				indices);
		glDrawElements(GL_TRIANGLES, nindices, GL_UNSIGNED_INT, NULL);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		aspect[0] = (float) height / width;
		aspect[1] = 1.0F;
		aspect[2] = 1.0F;
		glm_vec3_divs(aspect, 8.0F, aspect);
		glm_scale_make(world, aspect);
		hotbar_add();
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glUniformMatrix4fv(world_loc, 1, GL_FALSE, (float *) world);
		glBufferSubData(GL_ARRAY_BUFFER, 0,
				nvertices * sizeof(*vertices), 
				vertices);
		glDrawArrays(GL_TRIANGLES, 0, nvertices);
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
