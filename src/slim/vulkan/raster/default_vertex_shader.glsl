#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_tangent;
layout(location = 3) out vec2 out_uv;

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

struct ModelTransform {
    vec4 rotation;
    vec3 position;
    vec3 scale;
};

layout(push_constant) uniform Model {
    ModelTransform transform;
    ModelMaterialParams material_params;
} model;

//vec3 rotate(vec3 V, vec4 q) {
//    vec3 result = cross(q.xyz, V);
//    return V + 2.0f * (result * q.a + cross(q.xyz, result));
//}
//vec3 rotate(vec3 v, vec4 q) {
//    //    vec3 result = cross(q.xyz, V);
//    //  return V + 2.0f * (result * q.w + cross(q.xyz, result));
//    vec3 result = cross(q.xyz, v);
//    vec3 qqv = cross(q.xyz, result);
//    result = result * q.w + qqv;
//    result = result * 2.0f + v;
//    return result;
//    return v + 2.0f * cross(q.xyz, cross(q.xyz, v) + q.w * v);
//}

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

void main() {
    out_position = rotate(in_position * model.transform.scale, model.transform.rotation) + model.transform.position;
    out_normal = rotate(in_normal * model.transform.scale, model.transform.rotation);
    out_tangent = rotate(in_tangent * model.transform.scale, model.transform.rotation);
    out_uv = in_uv;

    gl_Position = camera.projection * camera.view * vec4(out_position, 1.0);
}