#pragma once

#include "./base.h"

#include <shaderc/shaderc.h>

namespace gpu {
    bool Shader::_create() {
        if (handle)
            return false;

        VkShaderModuleCreateInfo module_create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        module_create_info.codeSize = binary_size;
        module_create_info.pCode = binary_code;
        module_create_info.flags = 0;

        VK_CHECK(vkCreateShaderModule(device, &module_create_info, nullptr, &handle))

        return true;
    }

    bool Shader::destroy() {
        if (!handle)
            return false;

        vkDestroyShaderModule(device, handle, nullptr);
        handle = nullptr;

        return true;
    }

    bool Shader::operator!=(const Shader &other) const {
        if (&other == this)
            return false;

        if (stage != other.stage ||
            name != other.name ||
            entry_point_name != other.entry_point_name ||
            binary_code != other.binary_code ||
            binary_size != other.binary_size ||
            handle != other.handle)
            return true;

        return false;
    }

    bool Shader::operator==(const Shader &other) const {
        return !(*this != other);
    }

    const char* Shader::getDefaultName(VkShaderStageFlagBits stage) {
        switch (stage) {
            case VK_SHADER_STAGE_VERTEX_BIT: return "vertex_shader";
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return "tesselation_control_shader";
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return "tesselation_evaluation_shader";
            case VK_SHADER_STAGE_GEOMETRY_BIT: return "geometry_shader";
            case VK_SHADER_STAGE_FRAGMENT_BIT: return "fragment_shader";
            case VK_SHADER_STAGE_COMPUTE_BIT: return "compute_shader";
            default: return "unknown_shader";
        }
    }

    bool Shader::createFromSourceString(const char* glsl_source, VkShaderStageFlagBits stage, const char* name, const char* entry_point_name) {
        destroy();
        if (!name) name = getDefaultName(stage);
        this->name = (char*)name;
        this->stage = stage;
        this->entry_point_name = (char*)entry_point_name;

        size_t source_size = strlen(glsl_source);
        shaderc_shader_kind shader_kind;
        switch (stage) {
            case VK_SHADER_STAGE_VERTEX_BIT  : shader_kind = shaderc_glsl_vertex_shader; break;
            case VK_SHADER_STAGE_FRAGMENT_BIT: shader_kind = shaderc_glsl_fragment_shader; break;
            default:
                SLIM_LOG_DEBUG("Shader::createFromSourceString(): Unsupported shader stage for: %s\n", name)
                return false;
        }

        shaderc_compiler_t compiler = shaderc_compiler_initialize();
        shaderc_compile_options_t options = shaderc_compile_options_initialize();
        shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, 0);
        shaderc_compile_options_set_warnings_as_errors(options);

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
        if (num_warnings) SLIM_LOG_WARNING("Shader::FromSourceString(): %d warnings\n", num_warnings)
        if (num_errors) {
            SLIM_LOG_ERROR("Shader::createFromSourceString(): %d errors\n", num_errors)
            SLIM_LOG_ERROR("Shader::createFromSourceString(): Error: %s\n", shaderc_result_get_error_message(result))
            SLIM_LOG_ERROR("Shader::createFromSourceString(): Source:\n%s\n", glsl_source)
        }
        if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
            SLIM_LOG_ERROR("Shader::createFromSourceString(): Failed to compile shader %s :\n", name)

            shaderc_compile_options_release(options);
            shaderc_result_release(result);
            shaderc_compiler_release(compiler);
            return false;
        }

        const char *bytes = shaderc_result_get_bytes(result);;
        binary_size = shaderc_result_get_length(result);
        binary_code = (uint32_t*)os::getMemory(binary_size);
        memcpy(binary_code, bytes, binary_size);

        SLIM_LOG_DEBUG("Shader::createFromSourceString(): Compiled with size: %d", binary_size)

        shaderc_compile_options_release(options);
        shaderc_result_release(result);
        shaderc_compiler_release(compiler);

        return _create();
    }

    bool Shader::createFromSourceFile(const char* source_file, VkShaderStageFlagBits stage, const char* name, const char* entry_point_name) {
        destroy();
        if (!name) name = getDefaultName(stage);
        this->name = (char*)name;
        this->stage = stage;
        this->entry_point_name = (char*)entry_point_name;

        size_t file_size;
        const char* glsl_source = (char*)os::readEntireFile(source_file, &file_size);
        bool result = createFromSourceString(glsl_source, stage, name, entry_point_name);
        VirtualFree((void*)glsl_source, 0, MEM_RELEASE);
        return result;
    }

    bool Shader::createFromBinaryFile(const char* binary_file, VkShaderStageFlagBits stage, const char* name, const char* entry_point_name) {
        destroy();
        if (!name) name = getDefaultName(stage);
        this->name = (char*)name;
        this->stage = stage;
        this->entry_point_name = (char*)entry_point_name;

        binary_code = (uint32_t*)os::readEntireFile(binary_file, &binary_size);
        binary_size += binary_size % 4 ? (4 - (binary_size % 4)) : 0;

        SLIM_LOG_DEBUG("Shader::createFromBinaryFile(): Read with size: %d", binary_size)

        return _create();
    }

    bool Shader::createFromBinaryData(const uint32_t* binary_code, u64 binary_size, VkShaderStageFlagBits stage, const char* name, const char* entry_point_name) {
        destroy();
        if (!name) name = getDefaultName(stage);
        this->name = (char*)name;
        this->stage = stage;
        this->entry_point_name = (char*)entry_point_name;
        this->binary_size = binary_size;
        this->binary_code = (uint32_t*)binary_code;

        return _create();
    }
}


