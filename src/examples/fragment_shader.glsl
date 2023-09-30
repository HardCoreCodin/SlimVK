#version 450
#extension GL_ARB_separate_shader_objects : enable
#define pi (3.14159265f)
#define TAU (2.0f*pi)
#define UNIT_SPHERE_AREA_OVER_SIX ((4.0f*pi)/6.0f)
#define ONE_OVER_PI (1.0f/pi)
#define USE_ALBEDO_TEXTURE 1
#define USE_NORMAL_TEXTURE 2
#define EPS 0.0001f
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

vec3 rotate(vec3 V, vec4 q) {
    vec3 result = cross(q.xyz, V);
    return V + 2.0f * (result * q.a + cross(q.xyz, result));
}

vec4 getRotationFromAxisAndAngle(const vec3 axis, float radians) {
    radians *= 0.5f;
    return normalize(vec4(axis * sin(radians), cos(radians)));
}

vec4 getNormalRotation(const vec3 N, float magnitude) {
    return getRotationFromAxisAndAngle(normalize(vec3(N.z, 0, -N.x)), acos(N.y) * magnitude);
}

vec3 decodeNormal(const vec3 color) {
    return normalize(color.rbg * 2.0f - 1.0f);
}

float ggxTrowbridgeReitz_D(float roughness, float NdotH) { // NDF
    float a = roughness * roughness;
    float denom = NdotH * NdotH * (a - 1.0f) + 1.0f;
    return (
        a
        /
        (pi * denom * denom)
    );
}

float ggxSchlickSmith_G(float roughness, float NdotL, float NdotV) {
    float k = roughness * roughness * 0.5f;
    return (
        NdotV / max(mix(NdotV, 1.0f, k), EPS) *
        NdotL / max(mix(NdotL, 1.0f, k), EPS)
    );
}

vec3 schlickFresnel(float HdotL, vec3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - HdotL, 5.0f);
}

vec3 cookTorrance(float roughness, float NdotL, float NdotV, float NdotH, vec3 F) {
    float D = ggxTrowbridgeReitz_D(roughness, NdotH);
    float G = ggxSchlickSmith_G(roughness, NdotL, NdotV);
    return (
        F * (D * G
        / (
        4.0f * NdotL * NdotV
    )));
}

vec3 BRDF(vec3 albedo, float roughness, vec3 F0, float metalness, vec3 V, vec3 N, float NdotL, vec3 L) {
    vec3 Fs = vec3(0.0);
    vec3 Fd = albedo * ((1.0f - metalness) * ONE_OVER_PI);

    float NdotV = dot(N, V);
    if (NdotV <= 0.0f ||
        roughness <= 0)
        return Fs + Fd;

    vec3 H = normalize(L + V);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float HdotL = clamp(dot(H, L), 0.0, 1.0);
    vec3 F = schlickFresnel(HdotL, F0);
    Fs = cookTorrance(roughness, NdotL, NdotV, NdotH, F);
    Fd *= 1.0f - F;
    return Fs + Fd;
}

vec3 shadeFromLight(Light light, vec3 P, vec3 N, vec3 C, vec3 albedo, float roughness, vec3 F0, float metalness) {
    vec3 L = light.position - P;
    float NdotL = dot(L, N);
    if (NdotL <= 0.f)
        return vec3(0.f);

    float Ld = length(L);
    NdotL /= Ld;
    vec3 V = normalize(C - P);

    vec3 brdf = BRDF(albedo, roughness, F0, metalness, V, N, NdotL, L);
    return light.color * (brdf * NdotL * light.intensity / (Ld * Ld));
}

void main() {
    vec3 albedo = model.material_params.albedo;
    if ((model.material_params.use_textures & USE_ALBEDO_TEXTURE) != 0) {
        albedo *= texture(albedo_texture, in_uv).rgb;
    }
    const vec3 C = lighting.camera_position;
    const vec3 P = in_position;
    const vec3 F0 = model.material_params.F0;
    const float roughness = model.material_params.roughness;
    const float metalness = model.material_params.metalness;
    vec3 N = in_normal;
    if ((model.material_params.use_textures & USE_NORMAL_TEXTURE) != 0) {
        vec3 sampled_normal = decodeNormal(texture(normal_texture, in_uv).rgb);
        N = rotate(N, getNormalRotation(sampled_normal, model.material_params.normal_strength));
//        N = getNormalRotation(sampled_normal, 10.0).rgb;
    }
    vec3 color = (
        shadeFromLight(lighting.key_light , P, N, C, albedo, roughness, F0, metalness) +
        shadeFromLight(lighting.fill_light, P, N, C, albedo, roughness, F0, metalness) +
        shadeFromLight(lighting.rim_light , P, N, C, albedo, roughness, F0, metalness)
    );
    out_color = vec4(color, 1.0f);
}