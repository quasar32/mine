#pragma once

#define CGLM_OMIT_NS_FROM_STRUCT_API
#include <cglm/struct.h>

struct vertex {
    vec3s xyz; 
    vec2s uv;
    float lum;
};
