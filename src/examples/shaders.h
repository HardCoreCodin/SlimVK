#pragma once
#include "../slim/math/utils.h"
#include "../slim/vulkan/core/gpu.h"
using namespace gpu;

struct TriangleVertex {
    vec3 position{};
    vec3 normal;
    vec2 uv;
};
VertexDescriptor vertex_descriptor{
    sizeof(TriangleVertex),
    3, {
        _vec3,
        _vec3,
        _vec2
    }
};
VertexDescriptor line_vertex_descriptor{
    sizeof(vec3),
    1, { _vec3 }
};
const char* line_vertex_shader_source_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec3 out_position;
layout(push_constant) uniform Model {
    mat4 mvp;
    vec3 line_color;
} model;

void main() {
    gl_Position = model.mvp * vec4(in_position, 1.0);
    gl_Position.w += 0.00001f;
})VERTEX_SHADER";

const char* line_fragment_shader_source_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_color;
layout(push_constant) uniform Model {
    mat4 mvp;
    vec3 line_color;
} model;

void main() {
    out_color = vec4(model.line_color, 1.0f);
})FRAGMENT_SHADER";
const char* vertex_shader_file_path = "D:\\Code\\C++\\SlimVK\\assets\\shaders\\shader.vert.spv";
const char* fragment_shader_file_path = "D:\\Code\\C++\\SlimVK\\assets\\shaders\\shader.frag.spv";
