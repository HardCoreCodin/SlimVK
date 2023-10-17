#pragma once

#include "../core/gpu.h"
#include "../../math/vec3.h"
#include "../../math/mat4.h"

#include "./pipeline.h"

namespace default_material {
    using namespace gpu;

    struct MaterialParams {
        vec3 albedo;
        float roughness;
        vec3 F0;
        float metalness;
        float normal_strength;
        u32 use_textures;
    };

    struct PushConstant {
        alignas(16) mat4 object_to_world;
        alignas(16) MaterialParams material_params;
    };
    PushConstant push_constant{};
    PushConstantSpec push_constant_spec{{PushConstantRangeForVertexAndFragment(sizeof(PushConstant))}};

    VertexShader vertex_shader{};
    FragmentShader fragment_shader{};

    VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[]{
        DescriptorSetLayoutBindingForFragmentImageAndSampler(),
        DescriptorSetLayoutBindingForFragmentImageAndSampler()
    };
    constexpr u8 descriptor_set_binding_count = sizeof(descriptor_set_layout_bindings) / sizeof(VkDescriptorSetLayoutBinding);
    DescriptorSetLayout descriptor_set_layout{descriptor_set_layout_bindings, descriptor_set_binding_count, true};
    DescriptorPool descriptor_set_pool;
    VkDescriptorSet descriptor_sets[16];
    PipelineLayout pipeline_layout{};
    GraphicsPipeline pipeline{};

    bool create(const char* vertex_shader_file, const char* fragment_shader_file, const GPUImage *textures, u8 texture_count, RenderPass *render_pass = &present::render_pass) {
        const u8 material_instances_count = texture_count / 2;

        if (pipeline.handle)
            return true;

        if (!vertex_shader.createFromSourceFile(vertex_shader_file, &vertex_descriptor))
            return false;

        if (!fragment_shader.createFromSourceFile(fragment_shader_file))
            return false;

        if (!descriptor_set_layout.create())
            return false;

        VkDescriptorSetLayout descriptor_set_layouts[]{ raster_render_pipeline::descriptor_set_layout.handle, descriptor_set_layout.handle};
        if (!pipeline_layout.create(descriptor_set_layouts, 2, &push_constant_spec))
            return false;

        if (!pipeline.create(render_pass->handle, pipeline_layout.handle, vertex_shader, fragment_shader))
            return false;

        if (!descriptor_set_pool.createAndAllocate(descriptor_set_layout, descriptor_sets, material_instances_count))
            return false;

        for (u8 i = 0; i < material_instances_count; i++) {
            textures[i * 2 + 0].writeDescriptor(descriptor_sets[i], 0);
            textures[i * 2 + 1].writeDescriptor(descriptor_sets[i], 1);
        }

        return true;
    }

    void destroy() {
        pipeline.destroy();
        pipeline_layout.destroy();
        descriptor_set_layout.destroy();
        descriptor_set_pool.destroy();
        fragment_shader.destroy();
        vertex_shader.destroy();
    }

    void bind(const GraphicsCommandBuffer &command_buffer) {
        default_material::pipeline.bind(command_buffer);
        default_material::pipeline_layout.bind(raster_render_pipeline::descriptor_sets[present::current_frame], command_buffer);
    }

    void bindTextures(const GraphicsCommandBuffer &command_buffer, u8 set_index) {
        default_material::pipeline_layout.bind(default_material::descriptor_sets[set_index], command_buffer, 1);
    }

    void setModel(const GraphicsCommandBuffer &command_buffer, const mat4 &object_to_world, const MaterialParams &material_params) {
        default_material::push_constant.object_to_world = object_to_world;
        default_material::push_constant.material_params = material_params;
        default_material::pipeline_layout.pushConstants(command_buffer, default_material::push_constant_spec.ranges[0], &default_material::push_constant);
    }
}