#pragma once

#include "../core/base.h"

//struct PipelineStagesSpec {
//    u8 readable_stages = VK_SHADER_STAGE_FRAGMENT_BIT;
//    u8 writable_stages = 0;
//};
//
//struct PipelineImageSpec {
//    PipelineBindingSpec specs[16];
//    u8 count = 0;
//};
//
//struct PipelineUniformSpec {
//    PipelineBindingSpec specs[4];
//    u8 count = 0;
//};
//
//struct PushConstantSpec {
//    VkShaderStageFlags stages = 0;
//    u8 size = 0;
//};
//
//struct PipelineLayoutSpec {
//    u8 image_count = 0;
//    u8 uniform_count = 0;
//    PipelineImageSpec images[16] = {};
//    PipelineUniformSpec uniforms[4] = {};
//};
//
//struct PipelineSpec {
//    PipelineLayoutSpec layouts[4];
//    PushConstantSpec push_constants[4];
//    u8 layouts_count = 1;
//    u8 push_constants_count = 0;
//};
//
//
//struct PipelineLayoutBindingSpecs {
//    PipelineLayoutBindingSpec specs[16] = {};
//    u8 count = 1;
//};
//
//enum class PipelineLayoutBindingType : u8 {
//    Invalid,
//    Uniform,
//    Sampler,
//    Image,
//    ImageSampler
//};
//    PipelineLayoutBindingType type = PipelineLayoutBindingType::Invalid;

//struct PipelineLayoutBindingSpec {
//    VkDescriptorType type;
//    VkShaderStageFlags stages = {};
//    u8 array_length = 1;
//};
//
//struct PipelineLayoutSpec {
//    PipelineLayoutBindingSpec bindings[32];
//    u8 bindings_count = 0;
//};
//
//struct PipelineSpec {
//    PipelineLayoutSpec layouts[4];
//    u8 layouts_count = 0;
//    PushConstantsLayout push_constants_layout = {};
//};