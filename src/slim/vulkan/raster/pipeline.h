#include "../core/pipeline.h"
//#include "./base.h"

template <
    u8 pipelines_capacity = 16,
    u8 pipeline_layouts_capacity = 16,
    u8 descriptor_set_layouts_capacity = 16,
    u8 push_counts_capacity = 16,
    u8 shaders_capacity = 32
>
struct GraphicsPipelinesManager {
    GraphicsPipeline pipelines[pipelines_capacity] = {};
    PipelineLayout pipeline_layouts[pipeline_layouts_capacity] = {};
    DescriptorSetLayout descriptor_set_layouts[descriptor_set_layouts_capacity] = {};
    GraphicsPipelineStage stages[shaders_capacity] = {};

    u16 pipelines_count = 0;
    u16 pipeline_layouts_count = 0;
    u16 descriptor_set_layouts_count = 0;
    u16 stages_count = 0;

    u16 findDescriptorSetLayout(const DescriptorSetLayoutSpec &spec) {
        for (u16 i = 0; i < descriptor_set_layouts_count; i++)
            if (descriptor_set_layout_specs[i] == spec)
                return i;

        return -1;
    }

    u16 addDescriptorSetLayout(const DescriptorSetLayoutSpec &spec, bool create = false) {
        u16 i = findDescriptorSetLayout(spec);
        if (i != (u16)(-1))
            return i;

        if (descriptor_set_layouts_count == descriptor_set_layouts_capacity)
            return -1;

        i = descriptor_set_layouts_count++;
        descriptor_set_layout_specs[i] = spec;

        if (create)
            descriptor_set_layouts[i].create(spec);

        return i;
    }

    u16 findPipelineLayout(const PipelineLayoutSpec &spec) {
        for (u16 i = 0; i < pipeline_layouts_count; i++)
            if (pipeline_layout_specs[i] == spec)
                return i;

        return -1;
    }

    u16 addPipelineLayout(const PipelineLayoutSpec &spec, bool create = false) {
        u16 pipeline_layout_index = findPipelineLayout(spec);
        if (pipeline_layout_index != (u16)(-1))
            return pipeline_layout_index;

        if (pipeline_layouts_count == pipeline_layouts_capacity)
            return -1;

        pipeline_layout_index = pipeline_layouts_count++;
        pipeline_layout_specs[pipeline_layout_index] = spec;

        if (create)
            createPipelineLayout(pipeline_layout_index);

        return pipeline_layout_index;
    }

    bool createPipelineLayout(u16 pipeline_layout_index) {
        PipelineLayoutSpec &spec{pipeline_layout_specs[pipeline_layout_index]};
        PushConstantSpec &push_constants_spec{push_constant_specs[spec.push_constant_spec_index]};
        VkDescriptorSetLayout handles[16];

        if (spec.descriptor_set_layout_count) {
            for (u16 i = 0; i < spec.descriptor_set_layout_count; i++) {
                u16 dsl_index = spec.descriptor_set_layout_indices[i];
                if (descriptor_set_layouts[dsl_index].create(descriptor_set_layout_specs[dsl_index]))
                    handles[i] = descriptor_set_layouts[dsl_index].handle;
                else
                    return false;
            }
        }

        return pipeline_layouts[pipeline_layout_index].create(
            spec.descriptor_set_layout_count ? handles : nullptr,
            spec.descriptor_set_layout_count,
            push_constants_spec.count ? push_constants_spec.ranges : nullptr,
            push_constants_spec.count
        );
    }

    u16 findPipeline(const PipelineSpec &spec) {
        for (u16 i = 0; i < pipelines_count; i++)
            if (pipeline_specs[i] == spec)
                return i;

        return -1;
    }

    u16 addPipeline(const PipelineSpec &spec, bool create = false) {
        u16 pipeline_index = findPipeline(spec);
        if (pipeline_index != (u16)(-1))
            return pipeline_index;

        if (pipelines_count == pipelines_capacity)
            return -1;

        pipeline_index = pipelines_count++;
        pipeline_specs[pipeline_index] = spec;

        if (create)
            createPipeline(pipeline_index);

        return pipeline_index;
    }

    bool createPipeline(u16 pipeline_index) {
        if (!render_pass) render_pass = &present::render_pass;
        PipelineSpec &spec{pipeline_specs[pipeline_index]};
        return pipelines[pipeline_index].create(
            *render_pass,
            pipeline_layouts[spec.layout_index],
            vertex_descriptor,
            stages[spec.vertex_shader_index],
            stages[spec.fragment_shader_index],
            spec.flags & (u16)PipelineFlag::IsWireframe
        );
    }

    bool destroyAll() {
        for (u32 p = 0; p < pipelines_count; p++) pipelines[p].destroy();
        for (u32 pl = 0; pl < pipeline_layouts_count; pl++) pipeline_layouts[pl].destroy();
        for (u32 dsl = 0; dsl < descriptor_set_layouts_count; dsl++) descriptor_set_layouts[dsl].destroy();
    }

    bool createAll(
        const PipelineSpec &pipeline_spec,
        const VertexDescriptor &pipeline_vertex_descriptor,
        const CompiledShader &vertex_shader,
        const CompiledShader &fragment_shader,
        bool is_wireframe = false,
        const RenderPass *render_pass = nullptr) {
        if (!render_pass) render_pass = &present::render_pass;
        spec = pipeline_spec;
        vertex_descriptor = pipeline_vertex_descriptor;
        destroy();
        for (u8 l = 0; l < spec.layouts_count; l++) {
            DescriptorSetLayout &descriptor_set_layout{descriptor_set_layouts[l]};
            PipelineLayoutSpec &layout_spec{spec.layouts[l]};
            for (u8 binding_index = 0; binding_index < layout_spec.bindings_count; binding_index++) {
                PipelineLayoutBindingSpec &binding{layout_spec.bindings[binding_index]};
                descriptor_set_layout.add(binding.type, binding.stages, binding_index, binding.array_length);
            }
            descriptor_set_layout.create();
        }
        layout.create(descriptor_set_layouts, &spec.push_constants_spec, spec.layouts_count);
        pipeline.create(*render_pass, layout, vertex_descriptor, vertex_shader, fragment_shader, is_wireframe);
    }

    void destroy() {
        pipeline.destroy();
        pipeline = {};
        layout.destroy();
        layout = {};
        for (u8 l = 0; l < spec.layouts_count; l++) {
            descriptor_set_layouts[l].destroy();
            spec.layouts[l] = {};
        }
        vertex_descriptor = {};
        spec = {};
    }
};