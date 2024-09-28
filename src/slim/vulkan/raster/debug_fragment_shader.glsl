#version 450
#extension GL_ARB_separate_shader_objects : enable

#define HAS_ALBEDO_MAP 1
#define HAS_NORMAL_MAP 2
#define DRAW_DEPTH 4
#define DRAW_POSITION 8
#define DRAW_NORMAL 16
#define DRAW_UVS 32
#define DRAW_ALBEDO 64
#define DRAW_IBL 128
#define DRAW_SHADOW_MAP 256
#define DRAW_SHADOW_MAP_PROJECTED 512

struct Light 
{
	vec4 color_and_intensity;
	vec4 position_or_direction_and_type;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform LightingUniform {
    mat4 directional_light_transform;
    vec4 camera_position_and_IBL_intensity;
    Light lights[4];
} lighting;

layout(set = 1, binding = 0) uniform sampler2D albedo_texture;
layout(set = 1, binding = 1) uniform sampler2D normal_texture;
layout(set = 2, binding = 0) uniform samplerCube radiance_map;
layout(set = 2, binding = 1) uniform samplerCube irradiance_map;
layout(set = 3, binding = 0) uniform sampler2D directional_shadow_map_texture;

struct ModelTransform {
    vec4 rotation;
    vec3 position;
    vec3 scale;
};
struct ModelMaterialParams {
    vec3 albedo;
    float roughness;
    vec3 F0;
    float metalness;
    float normal_strength;
    uint flags;
};
layout(push_constant) uniform Model {
    ModelTransform transform;
    ModelMaterialParams material_params;
} model;

vec3 encodeNormal(vec3 normal) {
    return normal * 0.5f + 0.5f;
}

vec3 decodeNormal(vec4 color) {
    vec3 N = vec3(color.r, color.g, color.b);
    return normalize(N * 2.0f - 1.0f);
}
vec3 toneMapped(vec3 color) {
    vec3 x = clamp(color - 0.004f, 0.0f, 1.0f);
    vec3 x2_times_sholder_strength = x * x * 6.2f;
    return (x2_times_sholder_strength + x*0.5f)/(x2_times_sholder_strength + x*1.7f + 0.06f);
}

vec3 rotateNormal(vec3 Ng, vec4 normal_sample, float magnitude) {
    vec3 Nm = normalize(normal_sample.xzy * 2.0f - 1.0f);
    float angle = magnitude * acos(Nm.y) * 0.5f;
    vec4 q = normalize(vec4(normalize(vec3(Nm.z, 0, -Nm.x)) * sin(angle), cos(angle)));
    vec3 T = cross(q.xyz, Ng);
    return Ng + 2.0f * (T * q.w + cross(q.xyz, T));
}

void main() {
    if      ((model.material_params.flags & DRAW_SHADOW_MAP) != 0) {
        float s = texture(directional_shadow_map_texture, in_position.xy * 0.5f + 0.5f).r;
        out_color = vec4(vec3(s), 1.0f);
    }
    else if ((model.material_params.flags & DRAW_SHADOW_MAP_PROJECTED) != 0) {
        vec4 directional_light_space_pos = lighting.directional_light_transform * vec4(in_position, 1.0);
	    vec3 projCoords = directional_light_space_pos.xyz / directional_light_space_pos.w;
        out_color = vec4(vec3(texture(directional_shadow_map_texture, projCoords.xy * 0.5 + 0.5).r), 1.0f);
    }
    else if ((model.material_params.flags & DRAW_DEPTH     ) != 0) out_color = vec4(vec3(max(in_position.z * 0.00001f, 1.0f)), 1.0f);
    else if ((model.material_params.flags & DRAW_POSITION  ) != 0) out_color = vec4(clamp(in_position, 0.0, 1.0f), 1.0f);
    else if ((model.material_params.flags & DRAW_UVS       ) != 0) out_color = vec4(fract(1000.0f + in_uv.x), fract(1000.0f + in_uv.y), 0.0f, 1.0f);
    else if ((model.material_params.flags & DRAW_ALBEDO    ) != 0) {
        if (( model.material_params.flags & HAS_ALBEDO_MAP ) != 0) {
            out_color = vec4(texture(albedo_texture, in_uv).rgb * model.material_params.albedo, 1.0f);
        } else {
            out_color = vec4(model.material_params.albedo, 1.0f);
        }
    } else if ((model.material_params.flags & DRAW_NORMAL) != 0) {
        vec3 N = normalize(in_normal);
        if ((model.material_params.flags & HAS_NORMAL_MAP) != 0) {
            N = normalize(rotateNormal(N, texture(normal_texture, in_uv), model.material_params.normal_strength));
        }
        out_color = vec4(encodeNormal(N), 1.0f);
    }
}