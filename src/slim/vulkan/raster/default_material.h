#pragma once

#include "../core/gpu.h"
#include "../../math/vec3.h"
#include "../../math/mat4.h"

#include "./pipeline.h"

namespace default_material {
    using namespace gpu;

    char default_vertex_shader_file[100];
    char default_fragment_shader_file[100];
    char debug_fragment_shader_file[100];
    String shader_files[]{
        String::getFilePath("default_vertex_shader.glsl", default_vertex_shader_file, __FILE__),
        String::getFilePath("default_fragment_shader.glsl", default_fragment_shader_file, __FILE__),
        String::getFilePath("debug_fragment_shader.glsl", debug_fragment_shader_file, __FILE__)
    };

    struct MaterialParams {
        vec3 albedo;
        float roughness;
        vec3 F0;
        float metalness;
        float normal_strength;
        u32 flags;
    };
    struct ModelTransform {
        alignas(16) vec4 rotation;
        alignas(16) vec3 position;
        alignas(16) vec3 scale;

        ModelTransform(const Transform &transform = {}) :
            rotation{
                transform.orientation.axis.x,
                transform.orientation.axis.y,
                transform.orientation.axis.z,
                transform.orientation.amount
            },
            position{transform.position},
            scale{transform.scale}
        {}
    };
    struct PushConstant {
        alignas(16) ModelTransform transform;
        alignas(16) MaterialParams material_params;
    };
    PushConstant push_constant{};
    PushConstantSpec push_constant_spec{{PushConstantRangeForVertexAndFragment(sizeof(PushConstant))}};

    VertexShader vertex_shader{};
    FragmentShader fragment_shader{};
    FragmentShader debug_shader{};

    VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[]{
        DescriptorSetLayoutBindingForFragmentImageAndSampler(),
        DescriptorSetLayoutBindingForFragmentImageAndSampler()
    };
    constexpr u8 descriptor_set_binding_count = sizeof(descriptor_set_layout_bindings) / sizeof(VkDescriptorSetLayoutBinding);
    DescriptorSetLayout descriptor_set_layout{descriptor_set_layout_bindings, descriptor_set_binding_count, true};
    DescriptorSetLayout ibl_descriptor_set_layout{descriptor_set_layout_bindings, descriptor_set_binding_count, true};
    DescriptorPool descriptor_set_pool;
    DescriptorPool ibl_descriptor_set_pool;
    VkDescriptorSet descriptor_sets[16];
    VkDescriptorSet ibl_descriptor_sets[16];
    PipelineLayout pipeline_layout{};
    GraphicsPipeline pipeline{};
    GraphicsPipeline debug_pipeline{};

    bool create(
        const GPUImage *textures, u8 texture_count, 
        const GPUImage *ibl_maps, u8 ibl_map_count, 
        RenderPass *render_pass = &present::render_pass) {
        const u8 material_instances_count = texture_count / 2;

        if (pipeline.handle)
            return true;

        if (!vertex_shader.createFromSourceFile(default_vertex_shader_file, &vertex_descriptor))
            return false;

        if (!fragment_shader.createFromSourceFile(default_fragment_shader_file))
            return false;

        if (!debug_shader.createFromSourceFile(debug_fragment_shader_file))
            return false;

        if (!descriptor_set_layout.create())
            return false;

        if (!ibl_descriptor_set_layout.create())
            return false;

        VkDescriptorSetLayout descriptor_set_layouts[]{ 
            raster_render_pipeline::descriptor_set_layout.handle, 
            descriptor_set_layout.handle, 
            ibl_descriptor_set_layout.handle
        };
        if (!pipeline_layout.create(
            descriptor_set_layouts, 
            sizeof(descriptor_set_layouts) / sizeof(VkDescriptorSetLayout), 
            &push_constant_spec))
            return false;

        if (!pipeline.create(render_pass->handle, pipeline_layout.handle, vertex_shader, fragment_shader))
            return false;

        if (!debug_pipeline.create(render_pass->handle, pipeline_layout.handle, vertex_shader, debug_shader))
            return false;

        if (!descriptor_set_pool.createAndAllocate(descriptor_set_layout, descriptor_sets, material_instances_count))
            return false;

        for (u8 i = 0; i < material_instances_count; i++) {
            textures[i * 2 + 0].writeDescriptor(descriptor_sets[i], 0);
            textures[i * 2 + 1].writeDescriptor(descriptor_sets[i], 1);
        }

        if (!ibl_descriptor_set_pool.createAndAllocate(ibl_descriptor_set_layout, ibl_descriptor_sets, ibl_map_count))
            return false;
        
        for (u8 i = 0; i < ibl_map_count; i++) {
            ibl_maps[i * 2 + 0].writeDescriptor(ibl_descriptor_sets[i], 0);
            ibl_maps[i * 2 + 1].writeDescriptor(ibl_descriptor_sets[i], 1);
        }

        return true;
    }

    void destroy() {
        debug_pipeline.destroy();
        pipeline.destroy();
        pipeline_layout.destroy();
        descriptor_set_layout.destroy();
        descriptor_set_pool.destroy();
        ibl_descriptor_set_layout.destroy();
        ibl_descriptor_set_pool.destroy();
        vertex_shader.destroy();
        fragment_shader.destroy();
        debug_shader.destroy();
    }

    void bind(const GraphicsCommandBuffer &command_buffer, bool debug = false) {
        if (debug)
            debug_pipeline.bind(command_buffer);
        else
            pipeline.bind(command_buffer);
        pipeline_layout.bind(raster_render_pipeline::descriptor_sets[present::current_frame], command_buffer);
    }

    void bindTextures(const GraphicsCommandBuffer &command_buffer, u8 material_instance_index) {
        pipeline_layout.bind(descriptor_sets[material_instance_index], command_buffer, 1);
    }

    void bindIBL(const GraphicsCommandBuffer &command_buffer, u8 ibl_index) {
        pipeline_layout.bind(ibl_descriptor_sets[ibl_index], command_buffer, 2);
    }

    void setModel(const GraphicsCommandBuffer &command_buffer, const Transform &transform, const MaterialParams &material_params, u8 flags = 0) {
        push_constant.transform = transform;
        push_constant.material_params = material_params;
        if (flags) push_constant.material_params.flags = flags;
        pipeline_layout.pushConstants(command_buffer, default_material::push_constant_spec.ranges[0], &default_material::push_constant);
    }
}