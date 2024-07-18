#include <cglm/cglm.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stb_image.h>
#include <math.h>
#include <noise1234.h>

#include "hotbar.h"
#include "object.h"
#include "map.h"
#include "misc.h"
#include "light.h"

#define MAX_PITCH (GLM_PI_2f - 0.01F)
#define MIN_PITCH (-MAX_PITCH)
#define SENSITIVITY 0.001F

#define MAX_VERTICES (24 * BLOCKS_IN_WORLD) 
#define MAX_INDICES (36 * BLOCKS_IN_WORLD) 

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

#define FOR_XYZ_ALL \
    for (x = 0; x < CHUNK_LEN; x++) \
        for (y = 0; y < CHUNK_LEN; y++) \
            for (z = 0; z < CHUNK_LEN; z++) 

#define ENABLE_ATTRIB(i, n, type, member) enable_attrib(i, n, \
        sizeof(type), offsetof(type, member))

#define MAX_DIGS (CHAR_BIT * sizeof(int)) 

#include <cglm/struct.h>

struct vertex {
    vec3 xyz; 
    vec2 uv;
    float lum;
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
    },
    [STONE] = {
        {7.0F, 0.0F},
        {7.0F, 0.0F},
        {7.0F, 0.0F},
        {7.0F, 0.0F},
        {7.0F, 0.0F},
        {7.0F, 0.0F},
    }
};

static float face_lums[6] = {
    0.9F, 0.9F, 0.7F, 1.0F, 0.8F, 0.8F
};

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

static ivec3 pos_itc;
static uint8_t *block_itc;
static int axis_itc;

static float yaw; 
static float pitch;

static int held_left;
static int held_right;
static GLFWwindow *wnd;

static void ivec3_to_vec3(ivec3 iv, vec3 v) {
    v[0] = iv[0];
    v[1] = iv[1];
    v[2] = iv[2];
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
    pitch = CLAMP(pitch, MIN_PITCH, MAX_PITCH);
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

static float get_air_lum(ivec3 pos) {
    uint8_t *lum;

    lum = get_lum(pos);
    return lum ? (*lum + 1) / 16.0F : 1.0F;
}

static float get_face_lum(ivec3 pos, int face) {
    ivec3 empty; 

    glm_ivec3_copy(pos, empty);
    empty[face / 2] += face_dir(face);
    return get_air_lum(empty) * face_lums[face]; 
}
 
static void gen_block_vertices(struct chunk *c) {
    struct vertex *v;
    int face, i, j;
    uint8_t *block;
    uint32_t *idx;
    int x, y, z;
    ivec3 pos;

    for (i = 0; i < BLOCKS_IN_CHUNK; i++) {
        x = i / CHUNK_LEN / CHUNK_LEN;
        y = i / CHUNK_LEN % CHUNK_LEN; 
        z = i % CHUNK_LEN;
        block = &c->blocks[x][y][z];
        if (!*block) {
            continue;
        }
        pos[0] = x + c->pos[0] * CHUNK_LEN;
        pos[1] = y;
        pos[2] = z + c->pos[2] * CHUNK_LEN;
        for (face = 0; face < 6; face++) {
            if (!((c->faces[x][y][z] >> face) & 1)) {
                continue;
            }
            for (j = 0; j < 6; j++) {
                idx = indices + nindices++;
                *idx = nvertices + cube_indices[j];
            }
            for (j = 0; j < 4; j++) {
                v = &vertices[nvertices++];
                *v = cube_vertices[face * 4 + j];
                v->xyz[0] += pos[0];
                v->xyz[1] += pos[1];
                v->xyz[2] += pos[2];
                glm_vec2_scale(v->uv, 0.999F, v->uv);
                glm_vec2_add(v->uv, block_uvs[*block][face], v->uv); 
                glm_vec2_divs(v->uv, 16.0F, v->uv);
                v->lum = get_face_lum(pos, face);
                if (block == block_itc) {
                    v->lum *= 1.2F;
                }
            }
        }
    }
}

static void gen_chunk(int cx, int cz) {
    struct chunk *c;
    int x, z;
    int y, yf;

    c = &map[cx][0][cz];
    c->pos[0] = cx;
    c->pos[2] = cz;
    for (x = 0; x < CHUNK_LEN; x++) {
        for (z = 0; z < CHUNK_LEN; z++) {
            yf = 4;
            for (y = 0; y < yf; y++) {
                c->blocks[x][y][z] = GRASS;
                if (x == 0 || z == 0) {
                    c->blocks[x][y][z] = STONE;
                }
            }
        }
    }
    mark_chunk_dirty(c);
}

static void gen_map(void) {
    int x, z;

    for (x = 0; x < NX_CHUNK; x++) {
        for (z = 0; z < NZ_CHUNK; z++) {
            gen_chunk(x, z);
        }
    }
}

static void get_block_coord(vec3 src, ivec3 dst) {
    dst[0] = roundf(src[0]);
    dst[1] = roundf(src[1]);
    dst[2] = roundf(src[2]);
}

static float get_item_lum(struct object *obj) {
    ivec3 iv;

    get_block_coord(obj->pos, iv);
    return get_air_lum(iv);
}

static void gen_items_vertices(void) {
    int i, j;
    struct vertex *v;
    float c, s, x, z;
    vec2 *uv;
    float light;

    for (i = 0; i < n_items; i++) {
        for (j = 0; j < 36; j++) {
            indices[nindices++] = cube_indices[j] + nvertices;
        }
        c = cosf(items[i].rot);
        s = sinf(items[i].rot);
        uv = block_uvs[items[i].id];
        light = get_item_lum(items + i);
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
            v->lum = face_lums[j / 4] * light;
        }
        items[i].rot += dt;
    }
}

static void gen_faces(struct chunk *c) {
    int x, y, z;

    FOR_XYZ_ALL {
        if (!c->blocks[x][y][z]) {
            continue;
        }
        c->faces[x][y][z] = FRONT | RIGHT | TOP; 
        if (x == 0) {
            c->faces[x][y][z] |= LEFT;
        } else if (c->blocks[x - 1][y][z]) {
            c->faces[x - 1][y][z] &= ~RIGHT;
        } else {
            c->faces[x][y][z] |= LEFT;
        }
        if (y == 0) {
            c->faces[x][y][z] |= BOTTOM;
        } else if (c->blocks[x][y - 1][z]) {
            c->faces[x][y - 1][z] &= ~TOP;
        } else {
            c->faces[x][y][z] |= BOTTOM;
        }
        if (z == 0) {
            c->faces[x][y][z] |= BACK;
        } else if (c->blocks[x][y][z - 1]) {
            c->faces[x][y][z - 1] &= ~FRONT;
        } else {
            c->faces[x][y][z] |= BACK;
        }
    }
}

static void clear_vertices(void) {
    nvertices = 0;
    nindices = 0;
}

static void gen_vertices(void) {
    struct chunk *c;
    int i;
    int x, z;

    for (i = 0; i < n_dirty_chunks; i++) {
        c = dirty_chunks[i];
        gen_faces(c);
        c->dirty = 0;
    }
    gen_lums();
    n_dirty_chunks = 0;
    clear_vertices();
    for (i = 0; i < N_CHUNKS; i++) {
        x = i / NX_CHUNK;
        z = i % NZ_CHUNK; 
        c = &map[x][0][z];
        gen_block_vertices(c);
    }
    gen_items_vertices();
}

static int vec3_min_idx(vec3 v) {
    return v[0] < v[1] ? 
           (v[0] < v[2] ? 0 : 2) :
           (v[1] < v[2] ? 1 : 2);
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
        block_itc = get_block(pos_itc);
        if (block_itc && *block_itc) {
            break;
        }
    } 
    ivec3_to_vec3(pos_itc, v);
    if (glm_vec3_distance2(v, eye) > 25.0F) {
        block_itc = NULL;
    }
}

static void mark_itc_dirty(void) {
    int cx, cz;
    int x0, x1;
    int z0, z1;
    struct chunk *c;

    cx = pos_itc[0] / CHUNK_LEN;
    cz = pos_itc[2] / CHUNK_LEN;
    x0 = MAX(cx - 2, 0);
    x1 = MIN(cx + 3, NX_CHUNK);
    z0 = MAX(cz - 2, 0); 
    z1 = MIN(cz + 3, NZ_CHUNK); 
    for (cx = x0; cx < x1; cx++) {
        for (cz = z0; cz < z1; cz++) {
            c = &map[cx][0][cz];
            mark_chunk_dirty(c);
        }
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
    ivec3_to_vec3(pos_itc, item->pos);
    glm_vec3_zero(item->vel); 
    item->id = *block_itc;
    item->rot = 0.0F;
    *block_itc = 0;
    mark_itc_dirty();
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

static void render_digit(int val, float x, float y) {
    vec2 uv, xy;

    uv[0] = (int) (val / 4) * 2 + val % 2 + 8.0F;
    uv[1] = val / 2 % 2;
    xy[0] = x;
    xy[1] = y;
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
                render_int_ralign(slot->n, i - 4.8F, -6.85F);
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

static void move_player(void) {
    player.vel[0] = 0.0F;
    player.vel[2] = 0.0F;
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
    glm_vec3_copy(player.pos, eye);
    eye[1] += 1.7F;
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

unsigned block_prog, crosshair_prog;
unsigned vaos[NUM_VAOS], bos[NUM_BOS];
int world_loc;
int tex_loc;
int proj_loc;

int main(void) {
    mat4 proj, view, world;
    stbi_uc *data;
    unsigned tex;

    int w, h, channels;
    vec3 center;
    vec3 aspect;
    int i;

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
    data = stbi_load("img/atlas.png", &w, &h, &channels, 4);
    if (!data) {
        die("stbi_load: %s\n", stbi_failure_reason());
    }
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
            GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 
            0, GL_RGBA, GL_UNSIGNED_BYTE, data); 
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
    glUseProgram(block_prog);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(tex_loc, 0);
    held_left = 0;
    held_right = 0;
    gen_map();
    while (!glfwWindowShouldClose(wnd)) {
        glfwPollEvents();
        update_dt();
        update_cam_dirs();
        move_player();
        update_hotbar_sel();
        update_eye();
        update_block_itc();
        for (i = 0; i < n_items; i++) {
            move(items + i);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.5F, 0.6F, 1.0F, 1.0F);
        glm_perspective_default(width / (float) height, proj);
        glm_vec3_add(eye, front, center);
        glm_lookat(eye, center, up, view);
        glm_mat4_mul(proj, view, world);
        gen_vertices();
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
        gen_hotbar_vertices();
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
