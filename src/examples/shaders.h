#pragma once
#include "../slim/math/utils.h"
#include "../slim/vulkan/core/gpu.h"
using namespace gpu;

#define TRIANGLE_COUNT 1
#define VERTEX_COUNT 3
#define INDEX_COUNT (TRIANGLE_COUNT * 3)

struct TriangleVertex { vec3 position{}; Color color; };
TriangleVertex vertices[VERTEX_COUNT] = {
    {{0, -0.5, 0}, ColorID::Red},
    {{0.5, 0.5, 0}, ColorID::Green},
    {{0, 0.5, 0}, ColorID::Blue}
};
u16 indices[3 * TRIANGLE_COUNT] = {0, 1, 2};
VertexDescriptor vertex_descriptor{
    sizeof(TriangleVertex),
    2, {_vec3, _vec3}
};

const char* vertex_shader_source_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 0) out vec3 out_color;
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    vec4 proj;
} ubo;
layout(push_constant) uniform PushConstants {
    mat4 model;
} push_constants;

void main() {
    gl_Position = ubo.model * vec4(in_position, 1.0);
    out_color = in_color;
})VERTEX_SHADER";

const char* fragment_shader_source_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_color;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(in_color, 1.0);
})FRAGMENT_SHADER";

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