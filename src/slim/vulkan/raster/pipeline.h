#pragma once

#include "../../math/mat4_constructurs.h"
#include "../../scene/scene.h"
#include "../../viewport/viewport.h"
#include "../core/gpu.h"

namespace raster_render_pipeline {
    using namespace gpu;

    struct LightUniform
    {
	    alignas(16) vec4 color_and_intensity;
	    alignas(16) vec4 position_or_direction_and_type;

        const LightUniform& operator=(const DirectionalLight &rhs) {
            vec3 direction = -Mat3(rhs.orientation).forward; 
            color_and_intensity = vec4(rhs.color.r, rhs.color.g, rhs.color.b, rhs.intensity);
            position_or_direction_and_type = vec4(direction.x, direction.y, direction.z, 0.0f);
            return *this;
        }

        const LightUniform& operator=(const PointLight &rhs) {
            color_and_intensity = vec4(rhs.color.r, rhs.color.g, rhs.color.b, rhs.intensity);
            position_or_direction_and_type = vec4(rhs.position.x, rhs.position.y, rhs.position.z, 1.0f);
            return *this;
        }
    };

    struct CameraUniform {
        alignas(16) mat4 view;
        alignas(16) mat4 proj;
    };
    struct LightsUniform {
        alignas(16) mat4 directional_light_transform;
        alignas(16) vec4 camera_position_and_IBL_intensity;
        alignas(16) LightUniform lights[4];
    };
    VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[]{
        DescriptorSetLayoutBindingForVertexUniformBuffer(),
        DescriptorSetLayoutBindingForFragmentUniformBuffer()
    };
    constexpr u8 descriptor_set_binding_count = sizeof(descriptor_set_layout_bindings) / sizeof(VkDescriptorSetLayoutBinding);
    DescriptorSetLayout descriptor_set_layout{descriptor_set_layout_bindings, descriptor_set_binding_count, true};
    DescriptorPool descriptor_set_pool{};
    VkDescriptorSet descriptor_sets[VULKAN_MAX_FRAMES_IN_FLIGHT];
    UniformBuffer camera_uniform_buffers[VULKAN_MAX_FRAMES_IN_FLIGHT];
    UniformBuffer lights_uniform_buffers[VULKAN_MAX_FRAMES_IN_FLIGHT];
    LightsUniform lights_uniform_data{};
    CameraUniform camera_uniform_data{};
    
    void update(const mat4 &projection_matrix, const mat4 &view_matrix, const vec3 &camera_position, const PointLight *point_lights, u8 point_lights_count, const DirectionalLight &directional_light, f32 IBL_intensity = 1.0f) {
        for (int i = 0; i < point_lights_count; i++) {
            lights_uniform_data.lights[i] = point_lights[i];        
        }
        lights_uniform_data.lights[point_lights_count] = directional_light;
        lights_uniform_data.camera_position_and_IBL_intensity = Vec4(camera_position, IBL_intensity);
        lights_uniform_data.directional_light_transform = directional_light.shadowMapMatrix();
        camera_uniform_data.view = view_matrix;
        camera_uniform_data.proj = projection_matrix;
        camera_uniform_buffers[present::current_frame].upload(&camera_uniform_data);
        lights_uniform_buffers[present::current_frame].upload(&lights_uniform_data);
    }

    void update(const Scene &scene, const Viewport &viewport) {
        update(Mat4(viewport.frustum.projection),
            Mat4(*viewport.camera).inverted(),
            viewport.camera->position,
            scene.point_lights,
            (u8)scene.counts.point_lights,
            scene.directional_lights[0]);
    }

    bool create() {
        if (descriptor_set_layout.handle)
            return true;

        if (!descriptor_set_layout.create())
            return false;

        if (!descriptor_set_pool.createAndAllocate(descriptor_set_layout, descriptor_sets, VULKAN_MAX_FRAMES_IN_FLIGHT))
            return false;

        for (u8 i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            camera_uniform_buffers[i].create(sizeof(CameraUniform));
            lights_uniform_buffers[i].create(sizeof(LightsUniform));
            camera_uniform_buffers[i].writeDescriptor(descriptor_sets[i], 0);
            lights_uniform_buffers[i].writeDescriptor(descriptor_sets[i], 1);
        }

        return true;
    }

    void destroy() {
        for (size_t i = 0; i < VULKAN_MAX_FRAMES_IN_FLIGHT; i++) {
            camera_uniform_buffers[i].destroy();
            lights_uniform_buffers[i].destroy();
        }
        descriptor_set_pool.destroy();
        descriptor_set_layout.destroy();
    }
}