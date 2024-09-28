#pragma once

#include "../core/gpu.h"
#include "../../math/mat3.h"

namespace skybox_renderer {
    using namespace gpu;

    const char* vertex_shader_string = R"VERTEX_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec3 in_position;

layout(push_constant) uniform PushConstant {
    mat4 camera_ray_matrix;
} push_constant;

void main() 
{
    vec2 vertices[3] = vec2[3](vec2(-1, -1), vec2(3, -1), vec2(-1, 3));
    vec2 uv = vertices[gl_VertexIndex];//vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    in_position = (push_constant.camera_ray_matrix * vec4(uv, 1, 0)).xyz;
    gl_Position = vec4(uv, 0, 1);
})VERTEX_SHADER";

    const char* fragment_shader_string = R"FRAGMENT_SHADER(#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_position;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform samplerCube skybox;

void main()
{
	out_color = texture(skybox, in_position);
})FRAGMENT_SHADER";

    struct PushConstant {
        alignas(16) mat4 camera_ray_matrix;
    };
    PushConstant push_constant;
    PushConstantSpec push_constant_spec{{PushConstantRangeForVertex(sizeof(PushConstant))}};
    
    VkDescriptorSetLayoutBinding descriptor_set_layout_binding{ DescriptorSetLayoutBindingForFragmentImageAndSampler() };
    DescriptorSetLayout descriptor_set_layout{&descriptor_set_layout_binding, 1, true};
    DescriptorPool descriptor_set_pool{};
    VkDescriptorSet *descriptor_sets = nullptr;

    VertexShader vertex_shader{};
    FragmentShader fragment_shader{};

    PipelineLayout pipeline_layout{};
    GraphicsPipeline pipeline{};

    bool create(const GPUImage *skybox_images, const u32 skybox_images_count = 1, RenderPass *render_pass = &present::render_pass) {
        if (descriptor_set_layout.handle)
            return true;

        if (!descriptor_set_layout.create())
            return false;

        descriptor_sets = new VkDescriptorSet[skybox_images_count];
        if (!descriptor_set_pool.createAndAllocate(descriptor_set_layout, descriptor_sets, skybox_images_count))
            return false;

        if (!vertex_shader.createFromSourceString(vertex_shader_string, nullptr, "skybox_vertex_shader"))
            return false;

        if (!fragment_shader.createFromSourceString(fragment_shader_string, "skybox_fragment_shader"))
            return false;

        if (!pipeline_layout.create(&descriptor_set_layout.handle, 1, &push_constant_spec))
            return false;

        if (!pipeline.create(render_pass->handle, pipeline_layout.handle, &vertex_shader, &fragment_shader))
            return false;

        for (u32 i = 0; i < skybox_images_count; i++)
            skybox_images[i].writeDescriptor(descriptor_sets[i], 0);

        return true;
    }

    void destroy() {
        pipeline.destroy();
        pipeline_layout.destroy();
        descriptor_set_layout.destroy();
        descriptor_set_pool.destroy();
        fragment_shader.destroy();
        vertex_shader.destroy();
        delete[] descriptor_sets;
    }

    void draw(const GraphicsCommandBuffer &command_buffer, const mat4 &camera_ray_matrix, u32 skybox_index = 0) {
        pipeline.bind(command_buffer);
        pipeline_layout.bind(descriptor_sets[skybox_index], command_buffer, 0);

        push_constant.camera_ray_matrix = camera_ray_matrix;
        pipeline_layout.pushConstants(command_buffer, push_constant_spec.ranges[0], &push_constant);

        vkCmdDraw(command_buffer.handle, 3, 1, 0, 0);
    }
}