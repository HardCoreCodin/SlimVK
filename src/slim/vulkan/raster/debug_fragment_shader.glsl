#version 450
#extension GL_ARB_separate_shader_objects : enable

#define HAS_ALBEDO_MAP 1
#define HAS_NORMAL_MAP 1
#define DRAW_DEPTH 4
#define DRAW_POSITION 8
#define DRAW_NORMAL 16
#define DRAW_UVS 32
#define DRAW_ALBEDO 64

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D albedo_texture;
layout(set = 1, binding = 1) uniform sampler2D normal_texture;

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

vec3 Cross(vec3 lhs, vec3 rhs) {
    return vec3(
    (lhs.y * rhs.z) - (lhs.z * rhs.y),
    (lhs.z * rhs.x) - (lhs.x * rhs.z),
    (lhs.x * rhs.y) - (lhs.y * rhs.x)
    );
}
vec3 rotate(vec3 v, vec4 q) {
    vec3 axis = q.xyz;
    float amount = q.w;
    vec3 result = Cross(axis, v);
    vec3 qqv = Cross(axis, result);
    result = (result * amount) + qqv;
    result = (result * 2.0f) +  v;

//    v = normalize(v);
//    vec3 axis = q.xyz;
//    float amount = q.w;
//    vec3 result = cross(axis, v);
//    vec3 qqv = cross(axis, result);
//    result = result * amount + qqv;
//    result = result * 2.0f + v;
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
    vec3 axis = -cross(N, Nt);
    float l = length(axis);
    axis /= l;
    float angle = asin(l) * magnitude;// acos(dot(N, Nt)) * magnitude;
    return quat_from_axis_angle(axis, angle);
//    return getRotationFromAxisAndAngle(normalize(vec3(N.z, 0, -N.x)), acos(N.y) * magnitude);
}
//vec3 rotateNormal(vec3 Nm, float magnitude, vec3 Ng) {
//    return rotate(Ng, getNormalRotation(Nm, 1.0f));//quat(vec3(0.0f, 0.0f, 1.0f), Nm));
//}


vec3 rotateNormal(vec3 Nm, float magnitude, vec3 Ng) {
    vec3 axis = normalize(vec3(Nm.z, 0, -Nm.x));
    float angle = acos(Nm.y) * magnitude;
    angle *= 0.5f;
    axis *= sin(angle);
    float amount = cos(angle);

    vec4 q;
    q.x = axis.x;
    q.y = axis.y;
    q.z = axis.z;
    q.w = amount;
    q *= 1.0f / sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
//    axis.x = q.x;
//    axis.y = q.y;
//    axis.z = q.z;
//    amount = q.w;

//    vec3 result = Cross(axis, Ng);
//    vec3 qqv = Cross(axis, result);
//    result = result * amount + qqv;
//    result = result * 2.0f +  Ng;

    return rotate(Ng, q);
}

// axis      = up ^ normal = [0, 1, 0] ^ [x, y, z] = [1*z - 0*y, 0*x - 0*z, 0*y - 1*x] = [z, 0, -x]
// cos_angle = up . normal = [0, 1, 0] . [x, y, z] = 0*x + 1*y + 0*z = y
// (Pre)Swizzle N.z <-> N.y
//vec3 rotateNormal(const vec3 normal, vec3 normal_sample, float magnitude) {
//    return rotate(normal, getNormalRotation(normal_sample, magnitude));
//}

vec3 decodeNormal(vec4 color) {
    vec3 N = vec3(color.r, color.g, color.b);
    return normalize(N * 2.0f - 1.0f);
}

//vec3 decodeNormal(const vec4 color) {
//    vec3 normal = color.xzy * 2.0f - 1.0f;
////    normal.y = -normal.y;
//    return normalize(normal);
//}

vec3 encodeNormal(const vec3 normal) {
    return normal * 0.5f + 0.5f;
}

vec3 toneMapped(vec3 color) {
    vec3 x = clamp(color - 0.004f, 0.0f, 1.0f);
    vec3 x2_times_sholder_strength = x * x * 6.2f;
    return (x2_times_sholder_strength + x*0.5f)/(x2_times_sholder_strength + x*1.7f + 0.06f);
}


void main() {
    if      ((model.material_params.flags & DRAW_DEPTH   ) != 0) out_color = vec4(vec3(max(in_position.z * 0.00001f, 1.0f)), 1.0f);
    else if ((model.material_params.flags & DRAW_POSITION) != 0) out_color = vec4(clamp(in_position, 0.0, 1.0f), 1.0f);
    else if ((model.material_params.flags & DRAW_UVS     ) != 0) out_color = vec4(fract(1000.0f + in_uv.x), fract(1000.0f + in_uv.y), 0.0f, 1.0f);
    else if ((model.material_params.flags & DRAW_ALBEDO  ) != 0) {
        if ((model.material_params.flags & HAS_ALBEDO_MAP) != 0) {
            out_color = vec4(texture(albedo_texture, in_uv).rgb * model.material_params.albedo, 1.0f);
        } else {
            out_color = vec4(model.material_params.albedo, 1.0f);
        }
    } else if ((model.material_params.flags & DRAW_NORMAL) != 0) {
        vec3 N = normalize(in_normal);
        if ((model.material_params.flags & HAS_NORMAL_MAP) != 0) {
            vec3 T = normalize(in_tangent);
            vec3 B = cross(T, N);
            N = normalize(mat3(T, B, N) * decodeNormal(texture(normal_texture, in_uv)));
//            N = normalize(rotateNormal(decodeNormal(texture(normal_texture, in_uv)), 1.0f, N));
        }
        out_color = vec4(texture(normal_texture, in_uv).rgb, 1.0f);
    }
}