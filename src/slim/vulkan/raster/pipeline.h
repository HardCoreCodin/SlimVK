#pragma once

#include "../../math/mat4_constructurs.h"
#include "../../scene/scene.h"
#include "../../viewport/viewport.h"
#include "../core/gpu.h"

namespace raster_render_pipeline {
    using namespace gpu;

    struct CameraUniform {
        alignas(16) mat4 view;
        alignas(16) mat4 proj;
    };
    struct LightsUniform {
        alignas(16) vec4 camera_position_and_IBL_intensity;
        alignas(16) Light key_light;
        alignas(16) Light fill_light;
        alignas(16) Light rim_light;
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

    void update(const mat4 &projection_matrix, const mat4 &view_matrix, const vec3 &camera_position, const Light *lights, u8 light_count, f32 IBL_intensity = 1.0f) {
        lights_uniform_data.key_light = lights[0];
        lights_uniform_data.fill_light = lights[1];
        lights_uniform_data.rim_light = lights[2];
        lights_uniform_data.camera_position_and_IBL_intensity = Vec4(camera_position, IBL_intensity);
        camera_uniform_data.view = view_matrix;
        camera_uniform_data.proj = projection_matrix;
        camera_uniform_buffers[present::current_frame].upload(&camera_uniform_data);
        lights_uniform_buffers[present::current_frame].upload(&lights_uniform_data);
    }

    void update(const Scene &scene, const Viewport &viewport) {
        update(Mat4(viewport.frustum.projection),
            Mat4(*viewport.camera).inverted(),
            viewport.camera->position,
            scene.lights,
            (u8)scene.counts.lights);
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