#pragma once

#include "./base.h"

const char* vertex_shader_source = R"VERTEX_SHADER(#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
})VERTEX_SHADER";

const char* fragment_shader_source = R"FRAGMENT_SHADER(#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
})FRAGMENT_SHADER";

const char* vertex_shader_file_path = "D:\\Code\\C++\\SlimVK\\assets\\shaders\\shader.vert.spv";
const char* fragment_shader_file_path = "D:\\Code\\C++\\SlimVK\\assets\\shaders\\shader.frag.spv";

namespace gpu {
    namespace pipeline {
        void PipelineStage::create(const shader::CompiledShader &compiled_shader) {
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

        void PipelineStage::destroy() {
            vkDestroyShaderModule(device, handle, nullptr);
        }

        void Pipeline::destroy() {
            if (handle) {
                vkDestroyPipeline(device, handle, nullptr);
                handle = nullptr;
            }
            if (layout) {
                vkDestroyPipelineLayout(device, layout, nullptr);
                layout = nullptr;
            }
        }

        bool Pipeline::create(const shader::CompiledShader &vertex_shader,
                              const shader::CompiledShader &fragment_shader,
                              bool is_wireframe) {

            vertex_stage.create(vertex_shader);
            fragment_stage.create(fragment_shader);

            VkPipelineShaderStageCreateInfo shaderStages[] = {vertex_stage.create_info, fragment_stage.create_info};

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 0;
            vertexInputInfo.vertexAttributeDescriptionCount = 0;

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;
            viewportState.pViewports = &present::main_viewport;
            viewportState.pScissors = &present::main_scissor;

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
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pColorBlendState = &blend;
            pipelineInfo.pDynamicState = &dynamic;
//            pipelineInfo.pDepthStencilState = &depth_stencil;
            pipelineInfo.layout = layout;
            pipelineInfo.renderPass = gpu::main_render_pass.handle;
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

        bool Pipeline::createFromBinaryFiles(
            const char* vertex_shader_binary_file,
            const char* fragment_shader_binary_file,
            const char *vertex_shader_name,
            const char *fragment_shader_name,
            const char *vertex_shader_entry_point_name,
            const char *fragment_shader_entry_point_name) {
            return create(
                shader::CreateCompiledShaderFromBinaryFile(
                    vertex_shader_binary_file,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    vertex_shader_name,
                    vertex_shader_entry_point_name
                    ),
                shader::CreateCompiledShaderFromBinaryFile(
                    fragment_shader_binary_file,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    fragment_shader_name,
                    fragment_shader_entry_point_name)
            );
        }

        bool Pipeline::createFromBinaryData(
            uint32_t* vertex_shader_binary_data, u64 vertex_shader_data_size,
            uint32_t* fragment_shader_binary_data, u64 fragment_shader_data_size,
            const char *vertex_shader_name,
            const char *fragment_shader_name,
            const char *vertex_shader_entry_point_name,
            const char *fragment_shader_entry_point_name) {
            return create(
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

        bool Pipeline::createFromSourceFiles(
            const char* vertex_shader_source_file,
            const char* fragment_shader_source_file,
            const char *vertex_shader_name,
            const char *fragment_shader_name,
            const char *vertex_shader_entry_point_name,
            const char *fragment_shader_entry_point_name) {
            return create(
                shader::CreateCompiledShaderFromSourceFile(
                    vertex_shader_source_file,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    vertex_shader_name,
                    vertex_shader_entry_point_name
                    ),
                shader::CreateCompiledShaderFromSourceFile(
                    fragment_shader_source_file,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    fragment_shader_name,
                    fragment_shader_entry_point_name)
            );
        }

        bool Pipeline::createFromSourceStrings(
            const char* vertex_shader_source_string,
            const char* fragment_shader_source_string,
            const char *vertex_shader_name,
            const char *fragment_shader_name,
            const char *vertex_shader_entry_point_name,
            const char *fragment_shader_entry_point_name) {
            return create(
                shader::CreateCompiledShaderFromSourceString(
                    vertex_shader_source_string,
                    VK_SHADER_STAGE_VERTEX_BIT,
                    vertex_shader_name,
                    vertex_shader_entry_point_name
                    ),
                shader::CreateCompiledShaderFromSourceString(
                    fragment_shader_source_string,
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    fragment_shader_name,
                    fragment_shader_entry_point_name
                    )
            );
        }
    }
}