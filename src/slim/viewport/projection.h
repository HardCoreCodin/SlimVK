#pragma once

#include "../math/vec3.h"


struct Projection {
    ProjectionParams params;
    vec3 scale;
    f32 shear;

    Projection(const ProjectionParams &params) {
        update(params);
    }

    Projection(const Projection &other) : Projection{params} {}

    void update(const ProjectionParams &new_params) {
        params = new_params;
        const float f = params.far_distance;
        const float n = params.near_distance;
        if (params.is_perspective)
        {
            scale.x = params.focal_length * params.height_over_width;
            scale.y = params.focal_length;
            scale.z = 1.0f / (f - n);
            shear = 2 * f * n * -scale.z;
            scale.z *= f + n;
            
            if (!params.to_cube) {
                scale.z *= 0.5f;
                shear *= 0.5f;
            }
        }
        else
        {
            scale.x = 2.0f / params.width;
            scale.y = 2.0f / params.height;
            scale.z = 2.0f / (f - n);
            shear = (f + n) / (n - f);
            if (!params.to_cube) {
                scale.z *= 0.5f;
                shear *= 0.5f;
                shear += 0.5f;
            }
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
