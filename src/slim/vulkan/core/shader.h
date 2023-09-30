#pragma once

#include "./base.h"

#include <shaderc/shaderc.h>

namespace gpu {
    CompiledShader CreateCompiledShaderFromSourceString(const char* glsl_source, VkShaderStageFlagBits type, const char* name = "shader", const char* entry_point_name = "main") {
//                char* s = (char*)source_string;
//                while (*s++);
        size_t source_size = strlen(glsl_source); //s - (char*)source_string - 1;

        CompiledShader compiled_shader{type, name, entry_point_name};

        shaderc_shader_kind shader_kind;
        switch (type) {
            case VK_SHADER_STAGE_VERTEX_BIT  : shader_kind = shaderc_glsl_vertex_shader; break;
            case VK_SHADER_STAGE_FRAGMENT_BIT: shader_kind = shaderc_glsl_fragment_shader; break;
            default:
                SLIM_LOG_DEBUG("CreateCompiledShaderFromSourceString(): Unsupported shader type for: %s\n", name)
                return compiled_shader;
        }

        shaderc_compiler_t compiler = shaderc_compiler_initialize();
        shaderc_compile_options_t options = shaderc_compile_options_initialize();
        shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, 0);
        shaderc_compile_options_set_warnings_as_errors(options);
//                shaderc_compile_options_set_target_spirv(options, shaderc_spirv_version_1_5);

        shaderc_compilation_result_t result = shaderc_compile_into_spv(
            compiler,
            glsl_source,
            source_size,
            shader_kind,
            name,
            entry_point_name,
            options);

        size_t num_warnings = shaderc_result_get_num_warnings(result);
        size_t num_errors = shaderc_result_get_num_errors(result);
        if (num_warnings) SLIM_LOG_WARNING("CreateCompiledShaderFromSourceString(): %d warnings\n", num_warnings)
        if (num_errors) {
            SLIM_LOG_ERROR("CreateCompiledShaderFromSourceString(): %d errors\n", num_errors)
            SLIM_LOG_ERROR("CreateCompiledShaderFromSourceString(): Error: %s\n", shaderc_result_get_error_message(result))
            SLIM_LOG_ERROR("CreateCompiledShaderFromSourceString(): Source:\n%s\n", glsl_source)
        }
        if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
            SLIM_LOG_ERROR("CreateCompiledShaderFromSourceString(): Failed to compile shader %s :\n", name)

            shaderc_compile_options_release(options);
            shaderc_result_release(result);
            shaderc_compiler_release(compiler);
            return compiled_shader;
        }

        const char *bytes = shaderc_result_get_bytes(result);;
        compiled_shader.size = shaderc_result_get_length(result);
        compiled_shader.code = (uint32_t*)os::getMemory(compiled_shader.size);
        memcpy(compiled_shader.code, bytes, compiled_shader.size);

//                compiled_shader.code = (uint32_t*)shaderc_result_get_bytes(result);
//                compiled_shader.size += compiled_shader.size % 4 ? (4 - (compiled_shader.size % 4)) : 0;

        SLIM_LOG_DEBUG("CreateCompiledShaderFromSourceString(): Compiled with size: %d", compiled_shader.size)

        shaderc_compile_options_release(options);
        shaderc_result_release(result);
        shaderc_compiler_release(compiler);

        return compiled_shader;

    }

    CompiledShader CreateCompiledShaderFromSourceFile(const char* source_file, VkShaderStageFlagBits type, const char* name = "shader", const char* entry_point_name = "main") {
        size_t file_size;
        const char* glsl_source = (char*)os::readEntireFile(source_file, &file_size);
        CompiledShader compiled_shader = CreateCompiledShaderFromSourceString(glsl_source, type, name, entry_point_name);
        VirtualFree((void*)glsl_source, 0, MEM_RELEASE);
        return compiled_shader;
    }

    CompiledShader CreateCompiledShaderFromBinaryFile(const char* binary_file, VkShaderStageFlagBits type, const char* name = "shader", const char* entry_point_name = "main") {
        CompiledShader compiled_shader{type, name, entry_point_name};
        compiled_shader.code = (uint32_t*)os::readEntireFile(binary_file, &compiled_shader.size);
        compiled_shader.size += compiled_shader.size % 4 ? (4 - (compiled_shader.size % 4)) : 0;

        SLIM_LOG_DEBUG("CreateCompiledShaderFromBinaryFile(): Read with size: %d", compiled_shader.size)

        return compiled_shader;
    }

//    struct Vertex {
//        vec2 pos;
//        vec3 color;
//    };
//
//    static VkVertexInputBindingDescription VertexBindingDescription{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
//    static VkVertexInputAttributeDescription VertexAttributeDescriptions[2]{
//        { 0, 0, VK_FORMAT_R32G32_SFLOAT,  (u32)(offsetof(Vertex, pos)) },
//        {0, 1, VK_FORMAT_R32G32B32_SFLOAT, (u32)(offsetof(Vertex, color))}
//    };
//    VkDescriptorSetLayout descriptorSetLayout;
//    VkDescriptorPool descriptorPool;
//    VkDescriptorSet descriptorSets[VULKAN_MAX_FRAMES_IN_FLIGHT];


}


