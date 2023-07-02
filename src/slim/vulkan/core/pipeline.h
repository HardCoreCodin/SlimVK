#pragma once

#include "./shader.h"
#include "./render_pass.h"
#include "./graphics.h"

namespace gpu {

    struct GraphicsPipelineStage {
        VkShaderModule handle;
        VkShaderModuleCreateInfo module_create_info;
        VkPipelineShaderStageCreateInfo create_info;

        void create(const CompiledShader &compiled_shader) {
            *this = {};

            module_create_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            module_create_info.codeSize = compiled_shader.size;
            module_create_info.pCode = compiled_shader.code;
            module_create_info.flags = 0;

            VK_CHECK(vkCreateShaderModule(device, &module_create_info, nullptr, &handle))

            create_info = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
            create_info.module = handle;
            create_info.stage = compiled_shader.type;
            create_info.pName = compiled_shader.entry_point_name;
            create_info.pSpecializationInfo = nullptr;
        }

        void destroy() {
            vkDestroyShaderModule(device, handle, nullptr);
            handle = nullptr;
        }
    };

    struct DescriptorSetLayout {
        VkDescriptorSetLayout handle;

        void destroy() {
            if (handle) {
                vkDestroyDescriptorSetLayout(device, handle, nullptr);
                handle = nullptr;
            }
        }

        void create(VkDescriptorType type, VkShaderStageFlags stages, u32 binding = 0) {
            VkDescriptorSetLayoutBinding layout_binding{};
            layout_binding.binding = binding;
            layout_binding.descriptorCount = 1;
            layout_binding.descriptorType = type;
            layout_binding.pImmutableSamplers = nullptr;
            layout_binding.stageFlags = stages;

            VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
            layout_info.bindingCount = 1;
            layout_info.pBindings = &layout_binding;

            VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &handle))
        }

        void createForVertexUniformBuffer(u32 binding = 0) {
            create(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, binding);
        }
    };

    struct PushConstantsLayout {
        u32 count = 0;
        u32 total_size = 0;
        VkPushConstantRange ranges[32];

        bool addForVertexAndFragment(u32 size, u32 offset = 0) {
            if ((total_size + size) > 128) {
                SLIM_LOG_ERROR("Failed to add push constant range, out of space")
                return false;
            }
            VkShaderStageFlags stageFlags{VK_SHADER_STAGE_VERTEX_BIT |
                                          VK_SHADER_STAGE_FRAGMENT_BIT};
            ranges[count++] = {stageFlags, offset, size};
            return true;
        }
    };

    struct PipelineLayout {
        VkPipelineLayout handle;

        void destroy() {
            if (handle) {
                vkDestroyPipelineLayout(device, handle, nullptr);
                handle = nullptr;
            }
        }

        void create(DescriptorSetLayout *descriptor_set_layout = nullptr, PushConstantsLayout *push_constants_layout = nullptr) {
            VkPipelineLayoutCreateInfo pipeline_layout_create_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            if (descriptor_set_layout) {
                pipeline_layout_create_info.setLayoutCount = 1;
                pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout->handle;
            }
            if (push_constants_layout) {
                pipeline_layout_create_info.pushConstantRangeCount = push_constants_layout->count;
                pipeline_layout_create_info.pPushConstantRanges = push_constants_layout->ranges;
            }

            VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &handle))
        }

        void pushConstants(const GraphicsCommandBuffer &command_buffer, VkPushConstantRange &range, const void *data) {
            vkCmdPushConstants(command_buffer.handle, handle, range.stageFlags, range.offset, range.size, data);
        }
    };

    struct GraphicsPipeline {
        VkPipeline handle;

        GraphicsPipelineStage vertex_stage;
        GraphicsPipelineStage fragment_stage;

        void bind(const GraphicsCommandBuffer &command_buffer) const {
            vkCmdBindPipeline(command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS, handle);
        }

        void destroy() {
            if (handle) {
                vkDestroyPipeline(device, handle, nullptr);
                handle = nullptr;
            }
        }

        bool create(
            const RenderPass &render_pass,
            const PipelineLayout pipeline_layout,
            const VertexDescriptor &vertex_descriptor,
            const CompiledShader &vertex_shader,
            const CompiledShader &fragment_shader,
            bool is_wireframe = false) {
            vertex_stage.create(vertex_shader);
            fragment_stage.create(fragment_shader);

            VkPipelineShaderStageCreateInfo shaderStages[] = {vertex_stage.create_info, fragment_stage.create_info};

            // Vertex input
            VkVertexInputBindingDescription binding_description;
            binding_description.binding = 0;  // Binding index
            binding_description.stride = vertex_descriptor.vertex_input_stride;
            binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // Move to next data entry for each vertex.

            // Process attributes
            VkVertexInputAttributeDescription vertex_input_attribute_descriptions[16];
            u32 vertex_input_offset = 0;
            for (u32 i = 0; i < vertex_descriptor.attribute_count; ++i) {
                // Setup the new attribute.
                const VertexAttributeType &type{vertex_descriptor.attribute_types[i]};
                VkVertexInputAttributeDescription attribute;
                attribute.location = i;
                attribute.binding = 0;
                attribute.offset = vertex_input_offset;
                attribute.format = type.format;

                // Push into the config's attribute collection and add to the stride.
                vertex_input_attribute_descriptions[i] = attribute;
                vertex_input_offset += type.size;
            }

            // Attributes
            VkPipelineVertexInputStateCreateInfo vertex_input_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
            vertex_input_info.vertexBindingDescriptionCount = 1;
            vertex_input_info.pVertexBindingDescriptions = &binding_description;
            vertex_input_info.vertexAttributeDescriptionCount = vertex_descriptor.attribute_count;
            vertex_input_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions;

            // Input assembly
            VkPipelineInputAssemblyStateCreateInfo input_assembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
            input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            input_assembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;
//            viewportState.pViewports = &present::viewport;
//            viewportState.pScissors = &present::scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//            rasterizer.depthBiasEnable = VK_FALSE;
//            rasterizer.depthBiasConstantFactor = 0.0f;
//            rasterizer.depthBiasClamp = 0.0f;
//            rasterizer.depthBiasSlopeFactor = 0.0f;

            VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampling.minSampleShading = 1.0f;
            multisampling.pSampleMask = 0;
            multisampling.alphaToCoverageEnable = VK_FALSE;
            multisampling.alphaToOneEnable = VK_FALSE;

            VkPipelineDepthStencilStateCreateInfo depth_stencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
            depth_stencil.depthTestEnable = VK_TRUE;
            depth_stencil.depthWriteEnable = VK_TRUE;
            depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depth_stencil.depthBoundsTestEnable = VK_FALSE;
            depth_stencil.stencilTestEnable = VK_FALSE;

//            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendAttachmentState blend_attachment {};
            blend_attachment.blendEnable = VK_TRUE;
            blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
            blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

            VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
            blend.logicOpEnable = VK_FALSE;
            blend.logicOp = VK_LOGIC_OP_COPY;
            blend.attachmentCount = 1;
            blend.pAttachments = &blend_attachment;
            blend.blendConstants[0] = 0.0f;
            blend.blendConstants[1] = 0.0f;
            blend.blendConstants[2] = 0.0f;
            blend.blendConstants[3] = 0.0f;

            VkDynamicState dynamic_states[2] = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };
            VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
            dynamic.dynamicStateCount = 2;
            dynamic.pDynamicStates = dynamic_states;

            VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertex_input_info;
            pipelineInfo.pInputAssemblyState = &input_assembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pColorBlendState = &blend;
            pipelineInfo.pDynamicState = &dynamic;
//            pipelineInfo.pDepthStencilState = &depth_stencil;
            pipelineInfo.layout = pipeline_layout.handle;
            pipelineInfo.renderPass = render_pass.handle;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

            VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle))

            vertex_stage.destroy();
            fragment_stage.destroy();

            return true;
        }

        bool createFromBinaryData(
            const RenderPass &render_pass,
            const PipelineLayout &pipeline_layout,
            const VertexDescriptor &vertex_descriptor,
            uint32_t* vertex_shader_binary_data, size_t vertex_shader_data_size,
            uint32_t* fragment_shader_binary_data, size_t fragment_shader_data_size,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
                pipeline_layout,
                vertex_descriptor,
                {
                    VK_SHADER_STAGE_VERTEX_BIT,
                    vertex_shader_name,
                    vertex_shader_entry_point_name,
                    vertex_shader_binary_data,
                    vertex_shader_data_size
                },
                {
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    fragment_shader_name,
                    fragment_shader_entry_point_name,
                    fragment_shader_binary_data,
                    fragment_shader_data_size
                }
            );
        }

        bool createFromBinaryFiles(
            const RenderPass &render_pass,
            const PipelineLayout &pipeline_layout,
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_binary_file,
            const char* fragment_shader_binary_file,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
                pipeline_layout,
                vertex_descriptor,
                CreateCompiledShaderFromBinaryFile(
                    vertex_shader_binary_file,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    vertex_shader_name,
                    vertex_shader_entry_point_name
                ),
                CreateCompiledShaderFromBinaryFile(
                    fragment_shader_binary_file,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    fragment_shader_name,
                    fragment_shader_entry_point_name)
            );
        }

        bool createFromSourceFiles(
            const RenderPass &render_pass,
            const PipelineLayout &pipeline_layout,
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_source_file,
            const char* fragment_shader_source_file,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
                pipeline_layout,
                vertex_descriptor,
                CreateCompiledShaderFromSourceFile(
                    vertex_shader_source_file,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    vertex_shader_name,
                    vertex_shader_entry_point_name
                ),
                CreateCompiledShaderFromSourceFile(
                    fragment_shader_source_file,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    fragment_shader_name,
                    fragment_shader_entry_point_name)
            );
        }

        bool createFromSourceStrings(
            const RenderPass &render_pass,
            const PipelineLayout &pipeline_layout,
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_source_string,
            const char* fragment_shader_source_string,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
                pipeline_layout,
                vertex_descriptor,
                CreateCompiledShaderFromSourceString(
                    vertex_shader_source_string,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    vertex_shader_name,
                    vertex_shader_entry_point_name
                ),
                CreateCompiledShaderFromSourceString(
                    fragment_shader_source_string,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    fragment_shader_name,
                    fragment_shader_entry_point_name
                )
            );
        }
    };



    struct DescriptorSets {
        VkDescriptorSet handles[16];
        u32 count;

        void bind(u32 set_index, const PipelineLayout &set_layout, const CommandBuffer &command_buffer) {
            vkCmdBindDescriptorSets(command_buffer.handle, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    set_layout.handle, 0, 1,
                                    &handles[set_index], 0, nullptr);
        }
    };

    struct DescriptorPool {
        VkDescriptorPool handle;

        void destroy() {
            if (handle) {
                vkDestroyDescriptorPool(device, handle, nullptr);
                handle = nullptr;
            }
        }

        void create(VkDescriptorType type, u32 size) {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = type;
            poolSize.descriptorCount = size;

            VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = size;

            VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &handle))
        }

        void createForUniformBuffers(u32 count) {
            create(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count);
        }

        void allocate(u32 count, const VkDescriptorSetLayout *descriptor_set_layouts, DescriptorSets &out_descriptor_sets) {
            VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocInfo.descriptorPool = handle;
            allocInfo.descriptorSetCount = count;
            allocInfo.pSetLayouts = descriptor_set_layouts;

            VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, out_descriptor_sets.handles))

            out_descriptor_sets.count = count;
        }
    };

}