#version 450
#extension GL_ARB_separate_shader_objects : enable
#define pi (3.14159265f)
#define TAU (2.0f*pi)
#define UNIT_SPHERE_AREA_OVER_SIX ((4.0f*pi)/6.0f)
#define ONE_OVER_PI (1.0f/pi)

#define HAS_ALBEDO_MAP 1
#define HAS_NORMAL_MAP 1
#define DRAW_DEPTH 4
#define DRAW_POSITION 8
#define DRAW_NORMAL 16
#define DRAW_UVS 32
#define DRAW_ALBEDO 64

#define EPS 0.0001f
struct Light {
    vec3 color;
    float intensity;
    vec3 position;
    uint flags;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(binding = 1) uniform LightingUniform {
    vec4 camera_position_and_IBL_intensity;
    Light key_light;
    Light fill_light;
    Light rim_light;
} lighting;

layout(set = 1, binding = 0) uniform sampler2D albedo_texture;
layout(set = 1, binding = 1) uniform sampler2D normal_texture;
layout(set = 2, binding = 0) uniform samplerCube radiance_map;
layout(set = 2, binding = 1) uniform samplerCube irradiance_map;

struct ModelMaterialParams {
    vec3 albedo;
    float roughness;
    vec3 F0;
    float metalness;
    float normal_strength;
    uint flags;
};

struct ModelTransform {
    vec4 rotation;
    vec3 position;
    vec3 scale;
};

layout(push_constant) uniform Model {
    ModelTransform transform;
    ModelMaterialParams material_params;
} model;


struct Radiance {
	vec3 Fd;
	vec3 Fs;
};

vec3 rotateNormal(vec3 Ng, vec4 normal_sample, float magnitude) {
    vec3 Nm = normalize(normal_sample.xzy * 2.0f - 1.0f);
    float angle = magnitude * acos(Nm.y) * 0.5f;
    vec4 q = normalize(vec4(normalize(vec3(Nm.z, 0, -Nm.x)) * sin(angle), cos(angle)));
    vec3 T = cross(q.xyz, Ng);
    return Ng + 2.0f * (T * q.w + cross(q.xyz, T));
}

vec3 decodeNormal(const vec4 color) {
    return normalize(color.xyz * 2.0f - 1.0f);
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


Radiance BRDF(vec3 albedo, vec3 V, vec3 N, float NdotL, vec3 L) {
	Radiance rad;
    rad.Fs = vec3(0.0);
    rad.Fd = albedo * ((1.0f - model.material_params.metalness) * ONE_OVER_PI);
	float NdotV = dot(N, V);
    if (NdotV > 0.0f && model.material_params.roughness > 0) {
		vec3 H = normalize(L + V);
		float NdotH = clamp(dot(N, H), 0.0, 1.0);
		float HdotL = clamp(dot(H, L), 0.0, 1.0);
		vec3 F = schlickFresnel(HdotL, model.material_params.F0);
		rad.Fs = cookTorrance(model.material_params.roughness, NdotL, NdotV, NdotH, F);
		rad.Fd *= 1.0f - F;
	}
    
	return rad;
}

vec3 shadeFromLight(Light light, vec3 P, vec3 N, vec3 C, vec3 albedo) {
    vec3 L = light.position - P;
    float NdotL = dot(L, N);
    if (NdotL <= 0.f)
        return vec3(0.f);

    float Ld = length(L);
    NdotL /= Ld;
    vec3 V = normalize(C - P);

    Radiance rad = BRDF(albedo, V, N, NdotL, L);
    return light.color * ((rad.Fd + rad.Fs) * NdotL * light.intensity / (Ld * Ld));
}

vec3 toneMapped(vec3 color) {
    vec3 x = clamp(color - 0.004f, 0.0f, 1.0f);
    vec3 x2_times_sholder_strength = x * x * 6.2f;
    return (x2_times_sholder_strength + x*0.5f)/(x2_times_sholder_strength + x*1.7f + 0.06f);
}

void main() {
    vec3 albedo = model.material_params.albedo;
    if ((model.material_params.flags & HAS_ALBEDO_MAP) != 0) {
        albedo *= texture(albedo_texture, in_uv).rgb;
    }
    const vec3 C = lighting.camera_position_and_IBL_intensity.xyz;
    const vec3 P = in_position;
    vec3 N = normalize(in_normal);
    if ((model.material_params.flags & HAS_NORMAL_MAP) != 0) {
        //vec3 T = normalize(in_tangent);
        //vec3 B = cross(T, N);
        //N = normalize(mat3(T, B, N) * decodeNormal(texture(normal_texture, in_uv)));
        N = normalize(rotateNormal(N, texture(normal_texture, in_uv), model.material_params.normal_strength));
    }
    vec3 color = (
        shadeFromLight(lighting.key_light , P, N, C, albedo) +
        shadeFromLight(lighting.fill_light, P, N, C, albedo) +
        shadeFromLight(lighting.rim_light , P, N, C, albedo)
    );
    
    const float IBL_intensity = lighting.camera_position_and_IBL_intensity.w;
    if (IBL_intensity != 0.0f) {
        vec3 V = normalize(C - P);
		Radiance rad = BRDF(albedo, V, N, 1.0f, N);
		rad.Fd *= texture(irradiance_map, N).rgb;
		rad.Fs *= texture(radiance_map, normalize(reflect(V, N))).rgb;
		color += (rad.Fd + rad.Fs) * IBL_intensity;
	} 
    
    out_color = vec4(toneMapped(color), 1.0f);
}


/*
vec4 quat_from_axis_angle(vec3 axis, float angle)
{
    vec4 qr;
    float half_angle = (angle * 0.5);
    qr.x = axis.x * sin(half_angle);
    qr.y = axis.y * sin(half_angle);
    qr.z = axis.z * sin(half_angle);
    qr.w = cos(half_angle);
    return qr;
}

vec4 quat_conj(vec4 q)
{
    return vec4(-q.x, -q.y, -q.z, q.w);
}

vec4 quat_mult(vec4 q1, vec4 q2)
{
    vec4 qr;
    qr.x = (q1.w * q2.x) + (q1.x * q2.w) + (q1.y * q2.z) - (q1.z * q2.y);
    qr.y = (q1.w * q2.y) - (q1.x * q2.z) + (q1.y * q2.w) + (q1.z * q2.x);
    qr.z = (q1.w * q2.z) + (q1.x * q2.y) - (q1.y * q2.x) + (q1.z * q2.w);
    qr.w = (q1.w * q2.w) - (q1.x * q2.x) - (q1.y * q2.y) - (q1.z * q2.z);
    return qr;
}

vec3 rotate_vertex_position(vec3 position, vec4 qr)
{
    vec4 qr_conj = quat_conj(qr);
    vec4 q_pos = vec4(position.x, position.y, position.z, 0);

    vec4 q_tmp = quat_mult(qr, q_pos);
    qr = quat_mult(q_tmp, qr_conj);

    return vec3(qr.x, qr.y, qr.z);
}

vec3 rotate(vec3 v, vec4 q) {
    v = normalize(v);
    vec3 axis = q.xyz;
    float amount = q.w;
    vec3 result = cross(axis, v);
    vec3 qqv = cross(axis, result);
    result = result * amount + qqv;
    result = result * 2.0f + v;
    return result;
    //
    //    return V + 2.0f * (result * q.a + cross(q.xyz, result));
    //    return v + 2.0f * (cross(q.xyz, cross(q.xyz, v ) + q.w * v));
    //    return v + 2.0f * (cross(q.xyz, cross(q.xyz, v)) + q.w * cross(q.xyz, v));
}

vec4 getRotationFromAxisAndAngle(vec3 axis, float radians) {
    radians *= 0.5f;
    vec4 q;
    q.xyz = sin(radians) * axis;
    q.w   = cos(radians);
    return normalize(q);
    //    return normalize(vec4(axis * sqrt(1.0f - cos_radians*cos_radians), cos_radians));
}
//vec3 rotate(vec3 v, vec4 q) {
////    return rotate_vertex_position(v, q);
//    //    vec3 result = cross(q.xyz, V);
//    //  return V + 2.0f * (result * q.w + cross(q.xyz, result));
//    return v + 2.0f * (cross(q.xyz, cross(q.xyz, v)) + q.w * cross(q.xyz, v));
////    return v + 2.0f * (cross(q.xyz, cross(q.xyz, v ) + q.w * v));
//}

vec4 quat(vec3 v1, vec3 v2) {
    //    vec4 q;
    //    vec3 a = cross(v1, v2);
    //    q.xyz = a;
    //    q.w = sqrt(dot(v1, v1) * dot(v2, v2)) + dot(v1, v2);
    return normalize(vec4(cross(v1, v2), sqrt(dot(v1, v1) * dot(v2, v2)) + dot(v1, v2)));
}

vec4 getNormalRotation(vec3 N, float magnitude) {
    vec3 Nt = vec3(0.0f, 1.0f, 0.0f);
    vec3 axis = cross(N, Nt);
    float l = length(axis);
    axis /= l;
    float angle = asin(l) * magnitude;// acos(dot(N, Nt)) * magnitude;
    return quat_from_axis_angle(axis, angle);
    //    return getRotationFromAxisAndAngle(normalize(vec3(N.z, 0, -N.x)), acos(N.y) * magnitude);
}
vec3 rotateNormal(vec3 Nm, float magnitude, vec3 Ng) {
    return rotate(Nm, getNormalRotation(Ng, 1.0f));//quat(vec3(0.0f, 0.0f, 1.0f), Nm));
}

// axis      = up ^ normal = [0, 1, 0] ^ [x, y, z] = [1*z - 0*y, 0*x - 0*z, 0*y - 1*x] = [z, 0, -x]
// cos_angle = up . normal = [0, 1, 0] . [x, y, z] = 0*x + 1*y + 0*z = y
// (Pre)Swizzle N.z <-> N.y
//vec3 rotateNormal(const vec3 normal, vec3 normal_sample, float magnitude) {
//    return rotate(normal, getNormalRotation(normal_sample, magnitude));
//}
*/