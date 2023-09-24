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

const char* vertex_shader_source_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec2 out_uv;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push_constants;

void main() {
    gl_Position = ubo.proj * ubo.view * push_constants.model * vec4(in_position, 1.0);
    out_normal = in_normal;
    out_uv = in_uv;
})VERTEX_SHADER";

const char* fragment_shader_source_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform sampler2D my_texture;

void main() {
    out_color = vec4(in_normal, 1.0f);
})FRAGMENT_SHADER";
//    out_color = vec4(texture(my_texture, in_uv).xyz, 1.0f);
const char* vertex_shader_file_path = "D:\\Code\\C++\\SlimVK\\assets\\shaders\\shader.vert.spv";
const char* fragment_shader_file_path = "D:\\Code\\C++\\SlimVK\\assets\\shaders\\shader.frag.spv";



const char* vertex_shader_source_string2 = R"VERTEX_SHADER(#version 450
layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);
vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
})VERTEX_SHADER";