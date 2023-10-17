#pragma once

#include "../math/vec3.h"


enum class ProjectionType {
    Orthographic = 0,
    PerspectiveGL,
    PerspectiveDX
};

struct Projection {
    vec3 scale;
    f32 shear;
    ProjectionType type;

    Projection(f32 focal_length, f32 height_over_width, f32 n, f32 f,
               ProjectionType projection_type = ProjectionType::Orthographic) :
        scale{1}, shear{0}, type{projection_type} {
        update(focal_length, height_over_width, n, f);
    }
    Projection(const Projection &other) : scale{other.scale}, shear{other.shear} {}


    void update(f32 focal_length, f32 height_over_width, f32 n, f32 f) {
        scale.x = focal_length * height_over_width;
        scale.y = focal_length;
        scale.z = 1.0f / (f - n);
        shear = scale.z * -n;
        if (type == ProjectionType::PerspectiveDX) {
            shear *= f;
            scale.z *= f;
        } else  if (type == ProjectionType::PerspectiveGL) {
            shear *= 2 * f;
            scale.z *= f + n;
        }
    }

    vec3 project(const vec3 &position) const {
        vec3 projected_position{
            position.x * scale.x,
            position.y * scale.y,
            position.z * scale.z + shear
        };
        return projected_position / position.z;
    }
};
