#pragma once

#include "./shader.h"
#include "./render_pass.h"

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

    struct GraphicsPipeline {
        GraphicsPipelineStage vertex_stage;
        GraphicsPipelineStage fragment_stage;

        VkPipeline handle;
        VkPipelineLayout layout;

        bool create(
            const RenderPass &render_pass,
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
                VkVertexInputAttributeDescription attribute;
                attribute.location = i;
                attribute.binding = 0;
                attribute.offset = vertex_input_offset;
                attribute.format = VERTEX_ATTRIBUTE_FORMATS[vertex_descriptor.attributes[i].format];

                // Push into the config's attribute collection and add to the stride.
                vertex_input_attribute_descriptions[i] = attribute;

                vertex_input_offset += vertex_descriptor.attributes[i].size;
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

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;
            viewportState.pViewports = &present::viewport;
            viewportState.pScissors = &present::scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f;
            rasterizer.depthBiasClamp = 0.0f;
            rasterizer.depthBiasSlopeFactor = 0.0f;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
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


            VkPipelineColorBlendStateCreateInfo blend{};
            blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
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
            VkPipelineDynamicStateCreateInfo dynamic{};
            dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamic.dynamicStateCount = 2;
            dynamic.pDynamicStates = dynamic_states;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 0;
            pipelineLayoutInfo.pushConstantRangeCount = 0;

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
                vertex_stage.destroy();
                fragment_stage.destroy();

//                throw std::runtime_error("failed to create pipeline layout!");
            }

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
            pipelineInfo.layout = layout;
            pipelineInfo.renderPass = render_pass.handle;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &handle) != VK_SUCCESS) {
                vkDestroyPipelineLayout(device, layout, nullptr);
                vertex_stage.destroy();
                fragment_stage.destroy();

                //throw std::runtime_error("failed to create graphics pipeline!");
                return false;
            }

            vertex_stage.destroy();
            fragment_stage.destroy();

            return true;
        }

        bool createFromBinaryData(
            const RenderPass &render_pass,
            const VertexDescriptor &vertex_descriptor,
            uint32_t* vertex_shader_binary_data, size_t vertex_shader_data_size,
            uint32_t* fragment_shader_binary_data, size_t fragment_shader_data_size,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
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
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_binary_file,
            const char* fragment_shader_binary_file,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
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
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_source_file,
            const char* fragment_shader_source_file,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
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
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_source_string,
            const char* fragment_shader_source_string,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main") {
            return create(
                render_pass,
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

        void destroy() {
            if (handle) {
                vkDestroyPipeline(device, handle, nullptr);
                handle = nullptr;
            }
            if (layout) {
                vkDestroyPipelineLayout(device, layout, nullptr);
                layout = nullptr;
            }
        }
    };
}