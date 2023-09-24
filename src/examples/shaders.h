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

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;

layout(binding = 0) uniform CameraUniform {
    mat4 view;
    mat4 projection;
} camera;

layout(push_constant) uniform Model {
    mat4 object_to_world;
    mat4 object_to_world_inverse_transposed;
} model;

void main() {
    out_position = (model.object_to_world * vec4(in_position, 1.0)).xyz;
    out_normal = (model.object_to_world_inverse_transposed * vec4(in_normal, 0.0)).xyz;
    out_uv = in_uv;

    gl_Position = camera.projection * camera.view * vec4(out_position, 1.0);
})VERTEX_SHADER";




const char* fragment_shader_source_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable
#define pi (3.14159265f)
#define TAU (2.0f*pi)
#define UNIT_SPHERE_AREA_OVER_SIX ((4.0f*pi)/6.0f)
#define ONE_OVER_PI (1.0f/pi)

struct Light {
    vec3 color;
    float intensity;
    vec3 position;
    uint flags;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform LightingUniform {
    vec3 camera_position;
    Light key_light;
    Light fill_light;
    Light rim_light;
} lighting;

layout(binding = 2) uniform sampler2D my_texture;

vec3 shadeFromLight(Light light, vec3 P, vec3 N, vec3 C) {
    vec3 L = light.position - P;
    float Ld = length(L);
    L /= Ld;

    float NdotL = dot(L, N);
    if (NdotL > 0.0) {
        vec3 V = normalize(C - P);
        vec3 H = normalize(L + V);
        float HdotV = clamp(dot(N, H), 0.0, 1.0);
        float Ks = pow(HdotV, 16.0);
        float Kd = ONE_OVER_PI;
        return light.color * ((Ks + Kd) * NdotL * light.intensity / (Ld * Ld));
    } else return vec3(0.0);
}

void main() {
    vec3 light_color = (
        shadeFromLight(lighting.key_light , in_position, in_normal, lighting.camera_position) +
        shadeFromLight(lighting.fill_light, in_position, in_normal, lighting.camera_position) +
        shadeFromLight(lighting.rim_light , in_position, in_normal, lighting.camera_position)
    );
    out_color = vec4(light_color, 1.0f);
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