#version 450
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

struct ModelMaterialParams {
    vec3 albedo;
    float roughness;
    vec3 F0;
    float metalness;
    float normal_strength;
    uint use_textures;
};

layout(push_constant) uniform Model {
    mat4 object_to_world;
    ModelMaterialParams material_params;
} model;

void main() {
    out_position = (model.object_to_world * vec4(in_position, 1.0)).xyz;
    out_normal = (transpose(inverse(model.object_to_world)) * vec4(in_normal, 0.0)).xyz;
    out_uv = in_uv;

    gl_Position = camera.projection * camera.view * vec4(out_position, 1.0);
}