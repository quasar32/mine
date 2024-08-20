#define CGLM_OMIT_NS_FROM_STRUCT_API
#include <cglm/struct.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stb_image.h>
#include <math.h>
#include <noise1234.h>
#include <unistd.h>
#include <sys/stat.h>

#include "hotbar.h"
#include "object.h"
#include "map.h"
#include "misc.h"
#include "light.h"
#include "render.h"
#include "dirty.h"
#include "heightmap.h"
#include "face.h"
#include "jobq.h"

#define MAX_PITCH (GLM_PI_2f - 0.01F)
#define MIN_PITCH (-MAX_PITCH)
#define SENSITIVITY 0.001F

#define MAX_VERTICES (24 * BLOCKS_IN_WORLD) 
#define MAX_INDICES (36 * BLOCKS_IN_WORLD) 

#define VAO_MAP       0
#define VAO_CROSSHAIR 1
#define NUM_VAOS      2

#define VBO_MAP       0
#define EBO_MAP       1
#define VBO_CROSSHAIR 2
#define EBO_CROSSHAIR 3
#define NUM_BOS       4 

#define ENABLE_ATTRIB(i, n, type, member) \
    enable_attrib(i, n, sizeof(type), offsetof(type, member))

#define MAX_DIGS (CHAR_BIT * sizeof(int)) 

#define CGLM_OMIT_NS_FROM_STRUCT_API
#include <cglm/struct.h>

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
    {{{-0.5F, 0.5F, 0.5F}}, {{1.0F, 0.0F}}},
    {{{-0.5F, 0.5F, -0.5F}}, {{0.0F, 0.0F}}},
    {{{-0.5F, -0.5F, -0.5F}}, {{0.0F, 1.0F}}},
    {{{-0.5F, -0.5F, 0.5F}}, {{1.0F, 1.0F}}},
    {{{0.5F, 0.5F, 0.5F}}, {{0.0F, 0.0F}}},
    {{{0.5F, -0.5F, 0.5F}}, {{0.0F, 1.0F}}},
    {{{0.5F, -0.5F, -0.5F}}, {{1.0F, 1.0F}}},
    {{{0.5F, 0.5F, -0.5F}}, {{1.0F, 0.0F}}},
    {{{0.5F, -0.5F, 0.5F}}, {{1.0F, 0.0F}}},
    {{{-0.5F, -0.5F, 0.5F}}, {{0.0F, 0.0F}}},
    {{{-0.5F, -0.5F, -0.5F}}, {{0.0F, 1.0F}}},
    {{{0.5F, -0.5F, -0.5F}}, {{1.0F, 1.0F}}},
    {{{0.5F, 0.5F, 0.5F}}, {{1.0F, 1.0F}}},
    {{{0.5F, 0.5F, -0.5F}}, {{1.0F, 0.0F}}},
    {{{-0.5F, 0.5F, -0.5F}}, {{0.0F, 0.0F}}},
    {{{-0.5F, 0.5F, 0.5F}}, {{0.0F, 1.0F}}},
    {{{0.5F, 0.5F, -0.5F}}, {{0.0F, 0.0F}}},
    {{{0.5F, -0.5F, -0.5F}}, {{0.0F, 1.0F}}},
    {{{-0.5F, -0.5F, -0.5F}}, {{1.0F, 1.0F}}},
    {{{-0.5F, 0.5F, -0.5F}}, {{1.0F, 0.0F}}},
    {{{0.5F, 0.5F, 0.5F}}, {{1.0F, 0.0F}}},
    {{{-0.5F, 0.5F, 0.5F}}, {{0.0F, 0.0F}}},
    {{{-0.5F, -0.5F, 0.5F}}, {{0.0F, 1.0F}}},
    {{{0.5F, -0.5F, 0.5F}}, {{1.0F, 1.0F}}},
};

#define XYZUV(x, y, z, u, v) \
    ((x) | ((y) << 5) | ((z) << 10) | ((u) << 15) | ((v) << 20))

#define XYZUVL(x, y, z, u, v, l) \
    (XYZUV((x), (y), (z), (u), (v)) | ((l) << 25))

static uint32_t xyzuv[6][6] = {
    {
        XYZUV(0, 1, 1, 1, 0),
        XYZUV(0, 1, 0, 0, 0),
        XYZUV(0, 0, 0, 0, 1),
        XYZUV(0, 0, 0, 0, 1),
        XYZUV(0, 0, 1, 1, 1),
        XYZUV(0, 1, 1, 1, 0),
    },
    {
        XYZUV(1, 1, 1, 0, 0),
        XYZUV(1, 0, 1, 0, 1),
        XYZUV(1, 0, 0, 1, 1),
        XYZUV(1, 0, 0, 1, 1),
        XYZUV(1, 1, 0, 1, 0),
        XYZUV(1, 1, 1, 0, 0),
    },
    {
        XYZUV(1, 0, 1, 1, 0),
        XYZUV(0, 0, 1, 0, 0),
        XYZUV(0, 0, 0, 0, 1),
        XYZUV(0, 0, 0, 0, 1),
        XYZUV(1, 0, 0, 1, 1),
        XYZUV(1, 0, 1, 1, 0),
    },
    {
        XYZUV(1, 1, 1, 1, 1),
        XYZUV(1, 1, 0, 1, 0),
        XYZUV(0, 1, 0, 0, 0),
        XYZUV(0, 1, 0, 0, 0),
        XYZUV(0, 1, 1, 0, 1),
        XYZUV(1, 1, 1, 1, 1),
    },
    {
        XYZUV(1, 1, 0, 0, 0),
        XYZUV(1, 0, 0, 0, 1),
        XYZUV(0, 0, 0, 1, 1),
        XYZUV(0, 0, 0, 1, 1),
        XYZUV(0, 1, 0, 1, 0),
        XYZUV(1, 1, 0, 0, 0),
    },
    {
        XYZUV(1, 1, 1, 1, 0),
        XYZUV(0, 1, 1, 0, 0),
        XYZUV(0, 0, 1, 0, 1),
        XYZUV(0, 0, 1, 0, 1),
        XYZUV(1, 0, 1, 1, 1),
        XYZUV(1, 1, 1, 1, 0),
    }
};

static struct vertex square[] = {
    {{{0.5F, 0.5F}}, {.x = 1.0F}},
    {{{-0.5F, 0.5F}}, {}},
    {{{-0.5F, -0.5F}}, {.y = 1.0F}},
    {{{-0.5F, -0.5F}}, {.y = 1.0F}},
    {{{0.5F, -0.5F}}, {.x = 1.0F, .y = 1.0F}},
    {{{0.5F, 0.5F}}, {.x = 1.0F}},
};

static uint8_t crosshair_indices[] = {
    0, 1, 2,
    0, 2, 3,
    4, 5, 6,
    4, 6, 7,
};

static vec2s crosshair_vertices[] = {
    {.x = 0.05F, .y = 0.005F},
    {.x = -0.05F, .y = 0.005F},
    {.x = -0.05F, .y = -0.005F},
    {.x = 0.05F, .y = -0.005F},
    {.x = 0.005F, .y = 0.05F},
    {.x = -0.005F, .y = 0.05F},
    {.x = -0.005F, .y = -0.05F},
    {.x = 0.005F, .y = -0.05F},
};

static vec2s block_uvs[][6] = {
    [AIR] = {},
    [DIRT] = {
        {.x = 0.0F}, 
        {.x = 0.0F}, 
        {.x = 0.0F}, 
        {.x = 0.0F}, 
        {.x = 0.0F}, 
        {.x = 0.0F},
    },
    [GRASS] = {
        {.x = 2.0F},
        {.x = 2.0F},
        {.x = 0.0F},
        {.x = 1.0F},
        {.x = 2.0F},
        {.x = 2.0F},
    },
    [STONE] = {
        {.x = 7.0F},
        {.x = 7.0F},
        {.x = 7.0F},
        {.x = 7.0F},
        {.x = 7.0F},
        {.x = 7.0F},
    }
};

static float face_lums[6] = {
    0.9F, 0.9F, 0.7F, 1.0F, 0.8F, 0.8F
};

static struct vertex vertices[MAX_VERTICES];
static uint32_t indices[MAX_INDICES];
static int n_vertices;
static int n_indices;

static int width = 640;
static int height = 480;

static vec3s eye; 
static vec3s front;
static vec3s forw;
static vec3s up = {.y = 1.0F};
static vec3s right;

static ivec3s pos_itc;
static uint8_t *block_itc;
static int axis_itc;

static float yaw; 
static float pitch;

static int held_left;
static int held_right;
static GLFWwindow *wnd;

static unsigned vaos[NUM_VAOS], bos[NUM_BOS];

static int block_world_loc;

static vec3s vec3_ivec3(ivec3s v) {
    return (vec3s) {{v.x, v.y, v.z}};
}

static char *read_all_str(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return NULL;
    }
    struct stat st;
    if (fstat(fd, &st)) {
        goto close_fd;
    }
    char *buf = malloc(st.st_size + 1);
    if (!buf) {
        goto close_fd;
    }
    if (read(fd, buf, st.st_size) != st.st_size) {
        goto free_buf;
    }
    buf[st.st_size] = '\0';
    close(fd);
    return buf;
free_buf:
    free(buf);
close_fd:
    close(fd);
    return NULL;
}

static char *xasprintf(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *buf;
    if (vasprintf(&buf, fmt, ap) < 0) {
        die("xasprintf err\n");
    }
    va_end(ap);
    return buf;
}

static unsigned load_shader(GLenum type, const char *path) {
    unsigned shader;
    int status;
    char log[1024];
    const char *src;
    char *full;

    full = xasprintf("shader/%s", path);
    src = read_all_str(full);
    free(full);
    if (!src) {
        die("load_shader(%s): %s\n", path, strerror(errno));
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

static GLuint load_prog(const char *vs_path, const char *fs_path) {
    GLuint prog;
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
    off_x *= SENSITIVITY;
    off_y *= SENSITIVITY;
    yaw += off_x;
    pitch += off_y;
    yaw = fmodf(yaw, GLM_PIf * 2.0F);
    pitch = fclampf(pitch, MIN_PITCH, MAX_PITCH);
}

static void update_cam_dirs(void) {
    front.x = cosf(yaw) * cosf(pitch);
    front.y = sinf(pitch);
    front.z = sinf(yaw) * cosf(pitch);
    forw.x = cosf(yaw);
    forw.y = 0.0F;
    forw.z = sinf(yaw);
    right = vec3_cross(forw, up);
}

static float get_air_lum(ivec3s pos) {
    uint8_t *lum = get_lum(pos);
    return lum ? (*lum + 1) / 16.0F : 1.0F;
}

static int get_face_lum(ivec3s pos, int dir) {
    pos.raw[dir / 2] += face_dir(dir);
    uint8_t *lum = get_lum(pos);
    return lum ? *lum : 15;
}

static void block_gen_vertices(struct chunk *chunk, ivec3s local_pos) {
    uint8_t *block = get_local_block(chunk, local_pos); 
    if (!*block) {
        return;
    }
    ivec3s world_pos = local_to_world_pos(local_pos, chunk->pos);
    for (int dir = 0; dir < 6; dir++) {
        int face = *get_local_face(chunk, local_pos);
        if (!(face & (1 << dir))) {
            continue;
        }
        uint32_t x = local_pos.x;
        uint32_t y = local_pos.y;
        uint32_t z = local_pos.z;
        uint32_t u = block_uvs[*block][dir].u;
        uint32_t v = block_uvs[*block][dir].v;
        uint32_t l = get_face_lum(world_pos, dir);
        uint32_t *dst = &chunk->vertices[chunk->n_vertices];
        uint32_t *src = xyzuv[dir];
        for (int i = 0; i < 6; i++) {
            dst[i] = src[i] + XYZUVL(x, y, z, u, v, l); 
        }
        chunk->n_vertices += 6;
    }
}

static void enable_attrib(int i, int n, size_t stride, size_t offset) {
    glVertexAttribPointer(i, n, GL_FLOAT, 
            GL_FALSE, stride, (void *) offset);
    glEnableVertexAttribArray(i);
}

static void chunk_gen_vertices(struct chunk *chunk) {
    chunk->n_vertices = 0;
    ivec3s pos;
    FOR_LOCAL_POS (pos) {
        block_gen_vertices(chunk, pos);
    }
}

static void chunk_buf_verticies(struct chunk *chunk) {
    glBindVertexArray(chunk->vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
            chunk->n_vertices * sizeof(*chunk->vertices), 
            chunk->vertices);
}

static void world_gen_vertices(void) {
    do_dirty_jobs(DIRTY_VERTICES, chunk_gen_vertices);
    clear_dirty_all(DIRTY_VERTICES, chunk_buf_verticies); 
}

static void gen_heightmap(int cx, int cz) {
    struct heightmap *hm = &heightmaps[cx][cz];
    for (int lx = 0; lx < CHUNK_LEN; lx++) {
        for (int lz = 0; lz < CHUNK_LEN; lz++) {
            int wx = cx * CHUNK_LEN + lx; 
            int wz = cz * CHUNK_LEN + lz; 
            float nx = wx / (float) NX_BLOCKS;
            float nz = wz / (float) NZ_BLOCKS;
            float nv = (noise2(nx, nz) + 1.0F) / 2.0F; 
            int wh = iclamp(nv * NY_BLOCKS, 0, NY_BLOCKS);
            hm->heights[lx][lz] = wh;
            for (int cy = 0; cy < NY_CHUNKS; cy++) {
                struct chunk *c = &chunks[cx][cy][cz];
                int ly = wh - cy * CHUNK_LEN; 
                c->heightmap[lx][lz] = iclamp(ly, 0, CHUNK_LEN);
            }
        }
    }
}

static void gen_chunk(ivec3s pos) {
    struct chunk *c = chunk_pos_to_chunk(pos);
    c->pos = pos;
    for (int lx = 0; lx < CHUNK_LEN; lx++) {
        for (int lz = 0; lz < CHUNK_LEN; lz++) {
            for (int ly = 0; ly < c->heightmap[lx][lz]; ly++) {
                int wx = c->pos.x * CHUNK_LEN + lx; 
                int wy = c->pos.y * CHUNK_LEN + ly;
                int wz = c->pos.z * CHUNK_LEN + lz; 
                float nx = wx / (float) CHUNK_LEN;
                float ny = wy / (float) CHUNK_LEN;
                float nz = wz / (float) CHUNK_LEN;
                float nv = noise3(nx, ny, nz);
                if (nv > 0.0F) {
                    c->blocks[lx][ly][lz] = STONE; 
                }
            }
        }
    }
    mark_dirty_all(c);
}

static void gen_map(void) {
    ivec3s pos;
    for (int x = 0; x < NX_CHUNKS; x++) {
        for (int z = 0; z < NZ_CHUNKS; z++) {
            gen_heightmap(x, z);
        }
    }
    FOR_CHUNK_POS (pos) {
        gen_chunk(pos);
    }
}

static ivec3s get_block_coord(vec3s pos) {
    return (ivec3s) {
        .x = roundf(pos.x),
        .y = roundf(pos.y),
        .z = roundf(pos.z)
    };
}

static float get_item_lum(struct object *obj) {
    return get_air_lum(get_block_coord(obj->pos));
}

static void gen_items_vertices(void) {
    int i, j;
    struct vertex *v;
    float c, s, x, z;
    vec2s *uv;
    float light;

    n_vertices = 0;
    n_indices = 0;
    for (i = 0; i < n_items; i++) {
        for (j = 0; j < 36; j++) {
            indices[n_indices++] = cube_indices[j] + n_vertices;
        }
        c = cosf(items[i].rot);
        s = sinf(items[i].rot);
        uv = block_uvs[items[i].id];
        light = get_item_lum(items + i);
        for (j = 0; j < 24; j++) {
            v = vertices + n_vertices++;
            *v = cube_vertices[j];
            x = v->xyz.x;
            z = v->xyz.z;
            v->xyz.x = x * c - z * s; 
            v->xyz.z = x * s + z * c;
            v->xyz = vec3_divs(v->xyz, 4.0F);
            v->xyz = vec3_add(v->xyz, items[i].pos);
            v->uv = vec2_add(v->uv, uv[j / 4]);
            v->uv = vec2_divs(v->uv, 16.0F);
            v->lum = face_lums[j / 4] * light;
        }
        items[i].rot += dt;
    }
}

static void gen_vertices(void) {
    int n = dirties[DIRTY_FACES].n_chunks;
    double t0 = glfwGetTime();
    world_gen_lums();
    world_gen_faces();
    world_gen_vertices();
    gen_items_vertices();
    double t1 = glfwGetTime();
    if (n) {
        printf("%f\n", t1 - t0);
    }
}

static int vec3_min_idx(vec3s v) {
    return v.x < v.y ?
        (v.x < v.z ? 0 : 2) :
        (v.y < v.z ? 1 : 2);
}

static void find_block_itc(void) {
    vec3s delta;
    ivec3s step;
    vec3s disp;
    int i;
    vec3s v;
    struct chunk *chunk;
    ivec3s old_pos;
    uint8_t *old_block;

    old_pos = pos_itc;
    old_block = block_itc;
    pos_itc = get_block_coord(eye);
    for (i = 0; i < 3; i++) {
        delta.raw[i] = fabsf(1.0F / front.raw[i]);
        if (front.raw[i] < 0.0F) {
            step.raw[i] = -1;
            disp.raw[i] = eye.raw[i] - pos_itc.raw[i];
        } else {
            step.raw[i] = 1;
            disp.raw[i] = pos_itc.raw[i] - eye.raw[i];
        }
        disp.raw[i] = (disp.raw[i] + 0.5F) * delta.raw[i];
    }
    for (i = 0; i < 9; i++) {
        axis_itc = vec3_min_idx(disp);
        disp.raw[axis_itc] += delta.raw[axis_itc]; 
        pos_itc.raw[axis_itc] += step.raw[axis_itc];
        block_itc = get_block(pos_itc);
        if (block_itc && *block_itc) {
            break;
        }
    } 
    v = vec3_ivec3(pos_itc);
    if (vec3_distance2(v, eye) > 25.0F) {
        block_itc = NULL;
    } 
    if (block_itc != old_block) {
        if (block_itc) {
            chunk = world_pos_to_chunk(pos_itc);
            mark_dirty(chunk, DIRTY_VERTICES);
        }
        if (old_block) {
            chunk = world_pos_to_chunk(old_pos);
            mark_dirty(chunk, DIRTY_VERTICES);
        }
    }
}

static void mark_adj(ivec3s pos, int val, int axis) {
    ivec3s adj = ivec3_add_to_axis(pos, val, axis);
    struct chunk *chunk = chunk_pos_to_chunk_safe(adj);
    if (chunk) {
        mark_dirty(chunk, DIRTY_LUMS);
        mark_dirty(chunk, DIRTY_FACES);
        mark_dirty(chunk, DIRTY_VERTICES);
    }
}

static void mark_itc_dirty(void) {
    ivec3s pos = world_to_chunk_pos(pos_itc);
    struct chunk *chunk = chunk_pos_to_chunk(pos);
    mark_dirty_all(chunk);
    for (int axis = 0; axis < 3; axis++) {
        mark_adj(pos, -1, axis);
        mark_adj(pos, 1, axis); 
    }
    struct bounds bounds = {
        .min.x = imax(pos.x - 1, 0),
        .min.y = 0,
        .min.z = imax(pos.z - 1, 0),
        .max.x = imin(pos.x + 2, NX_CHUNKS),
        .max.y = NY_CHUNKS,
        .max.z = imin(pos.z + 2, NZ_CHUNKS)
    };
    FOR_BOUNDS (pos, bounds) {
        chunk = chunk_pos_to_chunk(pos);
        mark_dirty(chunk, DIRTY_LUMS);
        mark_dirty(chunk, DIRTY_VERTICES);
        mark_dirty_all(chunk);
    }
}

static void remove_block(void) {
    struct object *item;

    if (!block_itc) {
        return;
    }
    if (n_items == BLOCKS_IN_WORLD) {
        die("items oom\n");
    }
    item = items + n_items++;
    item->type = OBJ_ITEM;
    item->flags = 0;
    item->pos = vec3_ivec3(pos_itc);
    item->vel = vec3_zero();
    item->id = *block_itc;
    item->rot = 0.0F;
    *block_itc = 0;
    mark_itc_dirty();
}

static void add_square(vec2s uv, float div, vec2s xy, float scale, float lum) {
    int i;
    struct vertex *v;

    for (i = 0; i < 6; i++) { 
        v = vertices + n_vertices++;
        v->uv = vec2_add(uv, square[i].uv);
        v->uv = vec2_divs(v->uv, div);
        v->xyz.x = square[i].xyz.x * scale + xy.x;
        v->xyz.y = square[i].xyz.y * scale + xy.y;
        v->xyz.z = 0.0F;
        v->lum = lum;
    }
}

static void render_digit(int val, float x, float y) {
    vec2s uv, xy;

    uv.x = (int) (val / 4) * 2 + val % 2 + 8.0F;
    uv.y = val / 2 % 2;
    xy.x = x;
    xy.y = y;
    add_square(uv, 32.0F, xy, 0.3F, 1.0F);
}

static int *gen_digs(int val, int *digs) {
    int *dig;

    dig = digs;
    do {
        *dig++ = val % 10;
        val /= 10;
    } while (val);
    return dig;
}

static void render_int_ralign(int val, float x, float y) {
    int digs[MAX_DIGS];
    int *dig, *end;

    dig = digs;
    end = gen_digs(val, digs);
    while (dig < end) {
        render_digit(*dig++, x, y);
        x -= 0.3F;
    }
}

[[maybe_unused]]
static void render_int_lalign(int val, float x, float y) {
    int digs[MAX_DIGS];
    int *dig;

    dig = gen_digs(val, digs);
    while (dig > digs) {
        render_digit(*--dig, x, y);
        x += 0.3F;
    }
}

static void gen_hotbar_vertices(void) {
    int i;
    struct hotbar_slot *slot;
    vec2s uv;
    vec2s xy;
    float lum;

    n_vertices = 0;
    for (i = 0; i < 10; i++) {
        uv.x = 3;
        uv.y = 0;
        xy.x = i - 5;
        xy.y = -7;
        lum = (i == hotbar_sel) ? 2.5F : 1.0F;
        add_square(uv, 16.0F, xy, 1.0F, lum);
        slot = hotbar_slots + i;
        if (!slot->n) {
            continue;
        }
        xy.x = i - 5.0F; 
        xy.y = -7.0F;
        add_square(block_uvs[slot->id][5], 16.0F, 
                xy, 0.6F, 1.0F);
                render_int_ralign(slot->n, i - 4.8F, -6.85F);
    }
}

static void add_block(void) {
    ivec3s place_pos;
    struct prism obj_prism;
    struct prism block_prism;
    uint8_t *block;
    struct hotbar_slot *slot;
    int i;
    struct object *obj;

    if (!block_itc) {
        return;
    }
    place_pos = pos_itc;
    if (front.raw[axis_itc] < 0.0F) {
        place_pos.raw[axis_itc]++; 
    } else {
        place_pos.raw[axis_itc]--; 
    }
    block = get_block(place_pos);
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
    mark_itc_dirty();
    slot->n--;
    find_block_itc();
    for (i = 0; i < n_items; i++) {
        obj = items + i;
        get_world_prism(&obj_prism, obj);
        if (prism_collide(&obj_prism, &block_prism)) {
            obj->vel = GLMS_VEC3_ZERO;
            if (front.raw[axis_itc] < 0.0F) {
                obj->vel.raw[axis_itc] = 3.0F;
            } else {
                obj->vel.raw[axis_itc] = -3.0F;
            }
            obj->flags |= STUCK;
        }
    }
}

static void move_player(void) {
    vec2s horz = {};
    if (glfwGetKey(wnd, GLFW_KEY_W)) {
        horz.x += forw.x;
        horz.y += forw.z;
    } 
    if (glfwGetKey(wnd, GLFW_KEY_S)) {
        horz.x -= forw.x;
        horz.y -= forw.z;
    }
    if (glfwGetKey(wnd, GLFW_KEY_A)) {
        horz.x -= right.x;
        horz.y -= right.z;
    } 
    if (glfwGetKey(wnd, GLFW_KEY_D)) {
        horz.x += right.x;
        horz.y += right.z;
    }
    horz = vec2_normalize(horz);
    player.vel.x = horz.x * 5.0F;
    player.vel.z = horz.y * 5.0F;
    if (glfwGetKey(wnd, GLFW_KEY_SPACE) && 
            (player.flags & GROUNDED)) {
        player.vel = vec3_muladds(up, 8.0F, player.vel);
    }
    move(&player);
}

static void update_hotbar_sel(void) {
    int i;

    for (i = 0; i < 9; i++) {
        if (glfwGetKey(wnd, GLFW_KEY_1 + i)) {
            hotbar_sel = i;
        }
    } 
    if (glfwGetKey(wnd, GLFW_KEY_0)) {
        hotbar_sel = 9;
    }
}

static void update_eye(void) {
    eye = player.pos;
    eye.y += 1.7F;
}

static void update_block_itc(void) {
    find_block_itc();
    if (glfwGetMouseButton(wnd, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!held_left) {
            remove_block();
            held_left = 1;
        }
    } else {
        held_left = 0;
    }
    if (glfwGetMouseButton(wnd, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        if (!held_right) {
            add_block();
            held_right = 1;
        }
    } else {
        held_right = 0;
    }
}

static int chunk_visible(vec3s cmin, mat4s *mvp) {
    vec3s cmax = vec3_adds(cmin, CHUNK_LEN);
    vec3s min = {{INFINITY, INFINITY, INFINITY}};
    vec3s max = {{-INFINITY, -INFINITY, -INFINITY}};
    vec3s vs[8] = {
        {{cmin.x, cmin.y, cmin.z}},
        {{cmin.x, cmin.y, cmax.z}},
        {{cmin.x, cmax.y, cmin.z}},
        {{cmin.x, cmax.y, cmax.z}},
        {{cmax.x, cmin.y, cmin.z}},
        {{cmax.x, cmin.y, cmax.z}},
        {{cmax.x, cmax.y, cmin.z}},
        {{cmax.x, cmax.y, cmax.z}},
    };
    for (int i = 0; i < 8; i++) {
        vec4s pos = glms_vec4(vs[i], 1.0F);
        vec4s clip = mat4_mulv(*mvp, pos); 
        vec4s ndc = vec4_divs(clip, clip.w);
        min.x = fminf(min.x, ndc.x); 
        min.y = fminf(min.y, ndc.y); 
        min.z = fminf(min.z, ndc.z); 
        max.x = fmaxf(max.x, ndc.x); 
        max.y = fmaxf(max.y, ndc.y); 
        max.z = fmaxf(max.z, ndc.z); 
    }
    return min.x <= 1.0F && max.x >= -1.0F &&
           min.y <= 1.0F && max.y >= -1.0F &&
           min.z <= 1.0F && max.z >= -1.0F; 
}

static void draw_map(mat4s *vp) {
    ivec3s pos;
    struct chunk *chunk;

    FOR_CHUNK (pos, chunk) {
        vec3s fpos = {
            .x = pos.x * CHUNK_LEN - 0.5F,
            .y = pos.y * CHUNK_LEN - 0.5F,
            .z = pos.z * CHUNK_LEN - 0.5F,
        }; 
        if (chunk_visible(fpos, vp)) {
            mat4s model = glms_translate_make(fpos); 
            mat4s mvp = mat4_mul(*vp, model);
            glUniformMatrix4fv(block_world_loc, 1, GL_FALSE, 
                    (float *) &mvp);
            glBindVertexArray(chunk->vao);
            glDrawArrays(GL_TRIANGLES, 0, chunk->n_vertices); 
        }
    }
}

static void gen_chunk_buffers(void) {
    GLuint bufs[N_CHUNKS];
    GLuint vaos[N_CHUNKS];
    ivec3s pos;
    struct chunk *chunk;
    GLuint *buf, *vao;

    glGenBuffers(N_CHUNKS, bufs);
    glGenVertexArrays(N_CHUNKS, vaos);
    buf = bufs;
    vao = vaos;
    FOR_CHUNK (pos, chunk) {
        chunk->vao = *vao++;
        chunk->vbo = *buf++;
        glBindVertexArray(chunk->vao);
        glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
        glBufferData(GL_ARRAY_BUFFER,
                VERTICES_IN_CHUNK * sizeof(uint32_t),
                NULL, GL_DYNAMIC_DRAW);
        glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, 4, NULL);
        glEnableVertexAttribArray(0);
    }
}

static void update_items(void) {
    for (int i = 0; i < n_items; i++) {
        move(items + i);
    }
}

#if 0
static GLuint gen_png_texture(void) {
    static const char *paths[8] = {
        "dirt.png",
        "grass.png",
        "grass-dirt.png",
        "hotbar.png",
        "zero.png",
        "four.png",
        "eight.png",
        "stone.png",
    };
    int img_sz = 16 * 16 * 4;
    void *texels = malloc(8 * img_sz);
    char *dst = texels;
    for (int i = 0; i < 8; i++) {
        char *path = xasprintf("img/%s", paths[i]);
        int w, h, c;
        void *src = stbi_load(path, &w, &h, &c, 4);
        if (!src) {
            die("stbi_load(%s): %s\n", src, stbi_failure_reason());
        }
        if (w != 16 || h != 16) {
            die("invalid dimensions (%s)\n", path);
        }
        free(path);
        memcpy(dst, src, img_sz);
        dst += img_sz;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 16, 16, 8);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 
                    16, 16, 8, GL_RGBA, GL_UNSIGNED_BYTE, texels); 
    free(texels);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}
#endif

static GLuint gen_atlas(void) {
    int w, h, c;
    stbi_uc *data = stbi_load("img/atlas.png", &w, &h, &c, 4);
    if (!data) {
        die("stbi_load: %s\n", stbi_failure_reason());
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
            GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 
            0, GL_RGBA, GL_UNSIGNED_BYTE, data); 
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return tex;
}

int main(void) {
    mat4s proj, view, world;
    vec3s center;
    vec3s aspect;

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
    glfwSwapInterval(1);
    set_data_path();
    GLuint tex = gen_atlas();
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
    glBindVertexArray(vaos[VAO_CROSSHAIR]);
    glBindBuffer(GL_ARRAY_BUFFER, bos[VBO_CROSSHAIR]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crosshair_vertices), 
            crosshair_vertices, GL_STATIC_DRAW);
    enable_attrib(0, 2, sizeof(vec2s), 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bos[EBO_CROSSHAIR]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(crosshair_indices),
            crosshair_indices, GL_STATIC_DRAW);
    GLuint block_prog = load_prog("block.vert", "block.frag");
    GLuint prism_prog = load_prog("prism.vert", "prism.frag");
    GLuint crosshair_prog = load_prog("crosshair.vert", "crosshair.frag");
    GLint proj_loc = glGetUniformLocation(crosshair_prog, "proj");
    GLint prism_world_loc = glGetUniformLocation(prism_prog, "world");
    block_world_loc = glGetUniformLocation(block_prog, "world");
    glfwSetFramebufferSizeCallback(wnd, resize_cb);
    glfwSetInputMode(wnd, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(wnd, mouse_cb);
    glfwShowWindow(wnd);
    held_left = 0;
    held_right = 0;
    gen_map();
    gen_chunk_buffers();
    jobq_new();
    while (!glfwWindowShouldClose(wnd)) {
        glfwPollEvents();
        update_dt();
        update_cam_dirs();
        move_player();
        update_hotbar_sel();
        update_eye();
        update_block_itc();
        update_items();

        glClearColor(0.5F, 0.6F, 1.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        proj = glms_perspective_default(width / (float) height);
        center = vec3_add(eye, front);
        view = glms_lookat(eye, center, up);
        world = mat4_mul(proj, view);
        gen_vertices();
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glUseProgram(block_prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        draw_map(&world);
        glUseProgram(prism_prog);
        glUniformMatrix4fv(prism_world_loc, 1, GL_FALSE, (float *) &world);
        glBindVertexArray(vaos[VAO_MAP]);
        glBindBuffer(GL_ARRAY_BUFFER, bos[VBO_MAP]);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                n_vertices * sizeof(*vertices), 
                vertices);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bos[EBO_MAP]);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                n_indices * sizeof(*indices),
                indices);
        glDrawElements(GL_TRIANGLES, n_indices, GL_UNSIGNED_INT, NULL);
        aspect.x = (float) height / width;
        aspect.y = 1.0F;
        aspect.z = 1.0F;
        aspect = vec3_divs(aspect, 8.0F);
        world = glms_scale_make(aspect);
        gen_hotbar_vertices();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glUniformMatrix4fv(prism_world_loc, 1, GL_FALSE, (float *) &world);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                n_vertices * sizeof(*vertices), 
                vertices);
        glDrawArrays(GL_TRIANGLES, 0, n_vertices);
        glUseProgram(crosshair_prog);
        glBindVertexArray(vaos[VAO_CROSSHAIR]);
        aspect.x = (float) height / width;
        aspect.y = 1.0F;
        aspect.z = 1.0F;
        proj = glms_scale_make(aspect);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, (float *) &proj);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_BYTE, NULL); 
        glfwSwapBuffers(wnd);
    }
    return EXIT_SUCCESS;
}
