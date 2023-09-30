#version 450
#extension GL_ARB_separate_shader_objects : enable
#define pi (3.14159265f)
#define TAU (2.0f*pi)
#define UNIT_SPHERE_AREA_OVER_SIX ((4.0f*pi)/6.0f)
#define ONE_OVER_PI (1.0f/pi)
#define USE_ALBEDO_TEXTURE 1
#define USE_NORMAL_TEXTURE 2

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

layout(set = 1, binding = 0) uniform sampler2D albedo_texture;
layout(set = 1, binding = 1) uniform sampler2D normal_texture;

struct ModelMaterialParams {
    float roughness;
    uint use_textures;
};

layout(push_constant) uniform Model {
    mat4 object_to_world;
    ModelMaterialParams material_params;
} model;

vec3 shadeFromLight(Light light, vec3 P, vec3 N, vec3 C, vec3 Kd, float roughness) {
    vec3 L = light.position - P;
    float Ld = length(L);
    L /= Ld;

    float NdotL = dot(L, N);
    Kd *= ONE_OVER_PI;
    if (NdotL > 0.0) {
        vec3 V = normalize(C - P);
        vec3 H = normalize(L + V);
        float HdotV = clamp(dot(N, H), 0.0, 1.0);
        float s = pow(HdotV, 16.0);
        vec3 Ks = vec3(s);
        return light.color * ((mix(Ks, Kd, roughness)) * NdotL * light.intensity / (Ld * Ld));
    } else return vec3(0.0);
}

void main() {
    float roughness = model.material_params.roughness;
    vec3 Kd = model.material_params.use_textures == 1 ? texture(albedo_texture, in_uv).rgb : vec3(1.0);
    vec3 light_color = (
        shadeFromLight(lighting.key_light , in_position, in_normal, lighting.camera_position, Kd, roughness) +
        shadeFromLight(lighting.fill_light, in_position, in_normal, lighting.camera_position, Kd, roughness) +
        shadeFromLight(lighting.rim_light , in_position, in_normal, lighting.camera_position, Kd, roughness)
    );
    out_color = vec4(light_color, 1.0f);
}