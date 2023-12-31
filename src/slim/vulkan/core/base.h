#pragma once

#include <vulkan/vulkan.h>

#include <initializer_list>

#ifdef __linux__
//linux code goes here
#elif _WIN32
#include "../../platforms/win32_vulkan.h"
#endif

#include "../../core/logging.h"
#include "../../core/assertion.h"

#ifdef NDEBUG
    #define VK_CHECK(expr) expr
    #define VK_SET_DEBUG_OBJECT_NAME(object_type, object_handle, object_name)
    #define VK_SET_DEBUG_OBJECT_TAG(object_type, object_handle, tag_size, tag_data)
    #define VK_BEGIN_DEBUG_LABEL(command_buffer, label_name, color)
    #define VK_END_DEBUG_LABEL(command_buffer)
    #define VK_DEBUG_EXTENSIONS
    #define VK_VALIDATION_LAYERS
#else
    #define VK_CHECK(expr) SLIM_ASSERT_DEBUG((expr) == VK_SUCCESS)
    #define VK_SET_DEBUG_OBJECT_NAME(object_type, object_handle, object_name) _debug::setObjectName(object_type, object_handle, object_name)
    #define VK_SET_DEBUG_OBJECT_TAG(object_type, object_handle, tag_size, tag_data) _debug::setObjectTag(object_type, object_handle, tag_size, tag_data)
    #define VK_BEGIN_DEBUG_LABEL(command_buffer, label_name, color) _debug::beginLabel(command_buffer, label_name, color)
    #define VK_END_DEBUG_LABEL(command_buffer) _debug::endLabel(command_buffer)
    #define VK_DEBUG_EXTENSIONS VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    #define VK_VALIDATION_LAYERS "VK_LAYER_KHRONOS_validation"
#endif

#define VULKAN_MAX_SWAPCHAIN_FRAME_COUNT 3
#define VULKAN_MAX_FRAMES_IN_FLIGHT (VULKAN_MAX_SWAPCHAIN_FRAME_COUNT - 1)
#define VULKAN_MAX_PRESENTATION_MODES 8
#define VULKAN_MAX_SURFACE_FORMATS 512
#define VULKAN_MAX_MATERIAL_COUNT 1024
#define VULKAN_MAX_GEOMETRY_COUNT 4096

//#define VULKAN_MAX_UI_COUNT 1024

#define TEXTURE_NAME_MAX_LENGTH 512

// The maximum number of stages allowed (such as vertex, fragment, compute, etc.)
#define VULKAN_SHADER_MAX_STAGES 8

// The maximum number of textures allowed at the global level
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31

// The maximum number of textures allowed at the instance level
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31

// The maximum number of vertex input attributes allowed
#define VULKAN_SHADER_MAX_ATTRIBUTES 16

// The maximum number of uniforms and samplers allowed at the global, instance and local levels combined.
// It's probably more than will ever be needed
#define VULKAN_SHADER_MAX_UNIFORMS 128

// The maximum number of bindings per descriptor set
#define VULKAN_SHADER_MAX_BINDINGS 2

// The maximum number of push constant ranges for a shader
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32

namespace gpu {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;


    VkQueue transfer_queue;
    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue present_queue;

    unsigned int graphics_queue_family_index;
    unsigned int compute_queue_family_index;
    unsigned int transfer_queue_family_index;
    unsigned int present_queue_family_index;


    struct VertexAttributeType {
        VkFormat format;
        u8 size;
    };
    VertexAttributeType _f32{VK_FORMAT_R32_SFLOAT, 4};
    VertexAttributeType _vec2{VK_FORMAT_R32G32_SFLOAT, 8};
    VertexAttributeType _vec3{VK_FORMAT_R32G32B32_SFLOAT, 12};
    VertexAttributeType _vec4{VK_FORMAT_R32G32B32A32_SFLOAT, 16};
    VertexAttributeType _i8{VK_FORMAT_R8_SINT, 1};
    VertexAttributeType _u8{VK_FORMAT_R8_UINT, 1};
    VertexAttributeType _i16{VK_FORMAT_R16_SINT, 2};
    VertexAttributeType _u16{VK_FORMAT_R16_UINT, 2};
    VertexAttributeType _i32{VK_FORMAT_R32_SINT, 4};
    VertexAttributeType _u32{VK_FORMAT_R32_UINT, 4};

    struct VertexDescriptor {
        u32 vertex_input_stride;
        u32 attribute_count;
        VertexAttributeType attribute_types[16];
    };

    enum class BufferType {
        Unknown,
        Vertex,
        Index,
        Uniform,
        Staging,
        Read,
        Storage
    };

    enum class State {
        Ready,
        Recording,
        InRenderPass,
        Recorded,
        Submitted,
        UnAllocated
    };

    struct Attachment {
        enum class Type : u8 {
            Color = 1,
            Depth = 2,
            Stencil = 4
        };

        enum class Flag : u8 {
            Clear = 1,
            Load = 2,
            Store = 4,
            Present = 8,
            SourceIsView = 16
        };

        struct Config {
            Type type;
            u8 flags;
        };

        Config config;
        struct texture* texture;
    };

    struct Shader {
        VkShaderStageFlagBits stage = VK_SHADER_STAGE_ALL;
        char* name = nullptr;
        char* entry_point_name = nullptr;
        uint32_t* binary_code = nullptr;
        u64 binary_size = 0;
        VkShaderModule handle = nullptr;

        Shader() = default;
        Shader& operator=(const Shader &other) = delete;
        bool operator!=(const Shader &other) const;
        bool operator==(const Shader &other) const;

        bool destroy();

        static const char* getDefaultName(VkShaderStageFlagBits stage);

        bool createFromBinaryData(  const uint32_t* binary_conde,
                                    u64 binary_size,         VkShaderStageFlagBits stage, const char* name = nullptr, const char* entry_point_name = "main");
        bool createFromSourceString(const char* glsl_source, VkShaderStageFlagBits stage, const char* name = nullptr, const char* entry_point_name = "main");
        bool createFromSourceFile(  const char* source_file, VkShaderStageFlagBits stage, const char* name = nullptr, const char* entry_point_name = "main");
        bool createFromBinaryFile(  const char* binary_file, VkShaderStageFlagBits stage, const char* name = nullptr, const char* entry_point_name = "main");

    private:
        bool _create();
    };

    struct VertexShader : Shader {
        VertexDescriptor *vertex_descriptor = nullptr;

        VertexShader() = default;

        bool createFromBinaryData(  const uint32_t* binary_conde,
                                    u64 binary_size,         VertexDescriptor *vertex_descriptor, const char* name = nullptr, const char* entry_point_name = "main") { this->vertex_descriptor = vertex_descriptor; return Shader::createFromBinaryData(binary_conde, binary_size, VK_SHADER_STAGE_VERTEX_BIT, name, entry_point_name); }
        bool createFromSourceString(const char* glsl_source, VertexDescriptor *vertex_descriptor, const char* name = nullptr, const char* entry_point_name = "main") { this->vertex_descriptor = vertex_descriptor; return Shader::createFromSourceString(glsl_source, VK_SHADER_STAGE_VERTEX_BIT, name, entry_point_name); }
        bool createFromSourceFile(  const char* source_file, VertexDescriptor *vertex_descriptor, const char* name = nullptr, const char* entry_point_name = "main") { this->vertex_descriptor = vertex_descriptor; return Shader::createFromSourceFile(source_file, VK_SHADER_STAGE_VERTEX_BIT, name, entry_point_name); }
        bool createFromBinaryFile(  const char* binary_file, VertexDescriptor *vertex_descriptor, const char* name = nullptr, const char* entry_point_name = "main") { this->vertex_descriptor = vertex_descriptor; return Shader::createFromBinaryFile(binary_file, VK_SHADER_STAGE_VERTEX_BIT, name, entry_point_name); }
    };

    struct FragmentShader : Shader {
        FragmentShader() = default;

        bool createFromBinaryData(  const uint32_t* binary_conde,
                                          u64 binary_size,   const char* name = nullptr, const char* entry_point_name = "main") { return Shader::createFromBinaryData(binary_conde, binary_size, VK_SHADER_STAGE_FRAGMENT_BIT, name, entry_point_name); }
        bool createFromSourceString(const char* glsl_source, const char* name = nullptr, const char* entry_point_name = "main") { return Shader::createFromSourceString(glsl_source, VK_SHADER_STAGE_FRAGMENT_BIT, name, entry_point_name); }
        bool createFromSourceFile(  const char* source_file, const char* name = nullptr, const char* entry_point_name = "main") { return Shader::createFromSourceFile(source_file, VK_SHADER_STAGE_FRAGMENT_BIT, name, entry_point_name); }
        bool createFromBinaryFile(  const char* binary_file, const char* name = nullptr, const char* entry_point_name = "main") { return Shader::createFromBinaryFile(binary_file, VK_SHADER_STAGE_FRAGMENT_BIT, name, entry_point_name); }
    };

    VkPushConstantRange PushConstantRangeForVertexAndFragment(u8 size, u8 offset = 0) { return {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, offset, size}; }
    VkPushConstantRange PushConstantRangeForVertex(           u8 size, u8 offset = 0) { return {VK_SHADER_STAGE_VERTEX_BIT, offset, size}; }
    VkPushConstantRange PushConstantForFragment(              u8 size, u8 offset = 0) { return {VK_SHADER_STAGE_FRAGMENT_BIT, offset, size}; }

    struct PushConstantSpec {
        VkPushConstantRange ranges[32];
        u8 count = 0;

        PushConstantSpec(std::initializer_list<VkPushConstantRange> push_constant_ranges) {
            count = 0;
            u32 offset = 0;
            for (const auto &push_constant_range : push_constant_ranges) {
                ranges[count] = push_constant_range;
                ranges[count].offset = offset;
                offset += ranges[count].size;
                if (offset >= 128) {
                    SLIM_LOG_ERROR("Failed to add push constant range, out of space")
                    break;
                }
                if (++count == 32)
                    break;
            }
        }

        bool operator!=(const PushConstantSpec &other) const {
            if (count != other.count)
                return false;

            for (u8 i = 0; i < count; i++)
                if (ranges[i].stageFlags != other.ranges[i].stageFlags ||
                    ranges[i].size != other.ranges[i].size ||
                    ranges[i].offset != other.ranges[i].offset)
                    return true;

            return false;
        }

        bool operator==(const PushConstantSpec &other) const {
            return !(*this != other);
        }
    };

    VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(                                   VkShaderStageFlags stages, VkDescriptorType type, u8 array_length = 1) { return {(u32)-1, type, array_length, stages, nullptr};         };
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForUniformBuffer(                   VkShaderStageFlags stages,                        u8 array_length = 1) { return DescriptorSetLayoutBinding(stages,                              VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, array_length); }
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForImageAndSampler(                 VkShaderStageFlags stages,                        u8 array_length = 1) { return DescriptorSetLayoutBinding(stages,                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, array_length); }
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForFragmentUniformBuffer(                                                             u8 array_length = 1) { return DescriptorSetLayoutBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, array_length); }
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForFragmentImageAndSampler(                                                           u8 array_length = 1) { return DescriptorSetLayoutBinding(VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, array_length); }
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForVertexUniformBuffer(                                                               u8 array_length = 1) { return DescriptorSetLayoutBinding(VK_SHADER_STAGE_VERTEX_BIT,   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, array_length); }
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForVertexImageAndSampler(                                                             u8 array_length = 1) { return DescriptorSetLayoutBinding(VK_SHADER_STAGE_VERTEX_BIT,   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, array_length); }
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForVertexAndFragmentUniformBuffer(                                                    u8 array_length = 1) { return DescriptorSetLayoutBinding(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, array_length); }
    VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingForVertexAndFragmentImageAndSampler(                                                  u8 array_length = 1) { return DescriptorSetLayoutBinding(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, array_length); }

    bool areStringsEqual(const char *src, const char *trg) {
        bool equal = true;
        unsigned int s = 0;
        while (src[s]) {
            if (src[s] != trg[s]) {
                equal = false;
                break;
            }
            s++;
        }
        return equal && !trg[s];
    }

    bool allEnabledExtensionsAreAvailable(
        const char **enabled_extension_names, unsigned int enabled_extensions_count,
        VkExtensionProperties *available_extensions, unsigned int available_extensions_count) {
        for (unsigned int i = 0; i < enabled_extensions_count; ++i) {
            const char* enabled_extension_name = enabled_extension_names[i];

            bool found = false;
            for (unsigned int j = 0; j < available_extensions_count; ++j) {
                if (!areStringsEqual(enabled_extension_name, available_extensions[j].extensionName)) {
                    found = true;
                    SLIM_LOG_INFO("Found extension: %s...", enabled_extension_name);
                    break;
                }
            }

            if (!found) {
                SLIM_LOG_FATAL("Required extension is missing: %s", enabled_extension_name);
                return false;
            }
        }

        return true;
    }

    namespace present {
        u8 depth_channel_count; // The chosen depth format's number of channels
        VkFormat depth_format; // The chosen supported depth format
        VkSurfaceFormatKHR image_format;
    }

    namespace _instance {
        VkExtensionProperties available_extensions[256];
        unsigned int available_extensions_count = 0;

        const char* enabled_extension_names[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_WSI_SURFACE_EXTENSION_NAME,
            VK_DEBUG_EXTENSIONS
        };
        const unsigned int enabled_extensions_count = sizeof(enabled_extension_names) / sizeof(char*);

        VkLayerProperties available_validation_layers[8];
        unsigned int available_validation_layers_count = 0;

        const char* enabled_validation_layer_names[] = {
            VK_VALIDATION_LAYERS
        };
        const unsigned int enabled_validation_layers_count = sizeof(enabled_validation_layer_names) / sizeof(char*);
    }

    namespace _debug {
        bool init();
        void shutdown();
        bool setObjectName(VkObjectType object_type, void* object_handle, const char* object_name);
        bool setObjectTag(VkObjectType object_type, void* object_handle, u64 tag_size, const void* tag_data);
        bool beginLabel(VkCommandBuffer buffer, const char* label_name, Color color);
        bool endLabel(VkCommandBuffer buffer);
    }
}


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_types,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data) {

    switch (message_severity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: SLIM_LOG_ERROR(callback_data->pMessage); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: SLIM_LOG_WARNING(callback_data->pMessage); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: SLIM_LOG_INFO(callback_data->pMessage); break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: SLIM_LOG_TRACE(callback_data->pMessage); break;
    }
    return VK_FALSE;
}


const char* getVulkanResultString(
    VkResult result, // The result to get the string for
    bool get_extended // Indicates whether to also return an extended result
    ) {
    // Returns the string representation of result.
    // Returns the error code and/or extended error message in string form. Defaults to success for unknown result types.

    // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    // Success Codes
    switch (result) {
        default:
        case VK_SUCCESS: return !get_extended ? "VK_SUCCESS" : "VK_SUCCESS Command successfully completed";
        case VK_NOT_READY: return !get_extended ? "VK_NOT_READY" : "VK_NOT_READY A fence or query has not yet completed";
        case VK_TIMEOUT: return !get_extended ? "VK_TIMEOUT" : "VK_TIMEOUT A wait operation has not completed in the specified time";
        case VK_EVENT_SET: return !get_extended ? "VK_EVENT_SET" : "VK_EVENT_SET An event is signaled";
        case VK_EVENT_RESET: return !get_extended ? "VK_EVENT_RESET" : "VK_EVENT_RESET An event is unsignaled";
        case VK_INCOMPLETE: return !get_extended ? "VK_INCOMPLETE" : "VK_INCOMPLETE A return array was too small for the result";
        case VK_SUBOPTIMAL_KHR: return !get_extended ? "VK_SUBOPTIMAL_KHR" : "VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
        case VK_THREAD_IDLE_KHR: return !get_extended ? "VK_THREAD_IDLE_KHR" : "VK_THREAD_IDLE_KHR A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
        case VK_THREAD_DONE_KHR: return !get_extended ? "VK_THREAD_DONE_KHR" : "VK_THREAD_DONE_KHR A deferred operation is not complete but there is no work remaining to assign to additional threads.";
        case VK_OPERATION_DEFERRED_KHR: return !get_extended ? "VK_OPERATION_DEFERRED_KHR" : "VK_OPERATION_DEFERRED_KHR A deferred operation was requested and at least some of the work was deferred.";
        case VK_OPERATION_NOT_DEFERRED_KHR: return !get_extended ? "VK_OPERATION_NOT_DEFERRED_KHR" : "VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested and no operations were deferred.";
        case VK_PIPELINE_COMPILE_REQUIRED_EXT: return !get_extended ? "VK_PIPELINE_COMPILE_REQUIRED_EXT" : "VK_PIPELINE_COMPILE_REQUIRED_EXT A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";

       // Error codes
        case VK_ERROR_OUT_OF_HOST_MEMORY: return !get_extended ? "VK_ERROR_OUT_OF_HOST_MEMORY" : "VK_ERROR_OUT_OF_HOST_MEMORY A host memory allocation has failed.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return !get_extended ? "VK_ERROR_OUT_OF_DEVICE_MEMORY" : "VK_ERROR_OUT_OF_DEVICE_MEMORY A device memory allocation has failed.";
        case VK_ERROR_INITIALIZATION_FAILED: return !get_extended ? "VK_ERROR_INITIALIZATION_FAILED" : "VK_ERROR_INITIALIZATION_FAILED Initialization of an object could not be completed for implementation-specific reasons.";
        case VK_ERROR_DEVICE_LOST: return !get_extended ? "VK_ERROR_DEVICE_LOST" : "VK_ERROR_DEVICE_LOST The logical or physical device has been lost. See Lost Device";
        case VK_ERROR_MEMORY_MAP_FAILED: return !get_extended ? "VK_ERROR_MEMORY_MAP_FAILED" : "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed.";
        case VK_ERROR_LAYER_NOT_PRESENT: return !get_extended ? "VK_ERROR_LAYER_NOT_PRESENT" : "VK_ERROR_LAYER_NOT_PRESENT A requested layer is not present or could not be loaded.";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return !get_extended ? "VK_ERROR_EXTENSION_NOT_PRESENT" : "VK_ERROR_EXTENSION_NOT_PRESENT A requested extension is not supported.";
        case VK_ERROR_FEATURE_NOT_PRESENT: return !get_extended ? "VK_ERROR_FEATURE_NOT_PRESENT" : "VK_ERROR_FEATURE_NOT_PRESENT A requested feature is not supported.";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return !get_extended ? "VK_ERROR_INCOMPATIBLE_DRIVER" : "VK_ERROR_INCOMPATIBLE_DRIVER The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
        case VK_ERROR_TOO_MANY_OBJECTS: return !get_extended ? "VK_ERROR_TOO_MANY_OBJECTS" : "VK_ERROR_TOO_MANY_OBJECTS Too many objects of the type have already been created.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return !get_extended ? "VK_ERROR_FORMAT_NOT_SUPPORTED" : "VK_ERROR_FORMAT_NOT_SUPPORTED A requested format is not supported on this device.";
        case VK_ERROR_FRAGMENTED_POOL: return !get_extended ? "VK_ERROR_FRAGMENTED_POOL" : "VK_ERROR_FRAGMENTED_POOL A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
        case VK_ERROR_SURFACE_LOST_KHR: return !get_extended ? "VK_ERROR_SURFACE_LOST_KHR" : "VK_ERROR_SURFACE_LOST_KHR A surface is no longer available.";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return !get_extended ? "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" : "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
        case VK_ERROR_OUT_OF_DATE_KHR: return !get_extended ? "VK_ERROR_OUT_OF_DATE_KHR" : "VK_ERROR_OUT_OF_DATE_KHR A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return !get_extended ? "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" : "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
        case VK_ERROR_INVALID_SHADER_NV: return !get_extended ? "VK_ERROR_INVALID_SHADER_NV" : "VK_ERROR_INVALID_SHADER_NV One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return !get_extended ? "VK_ERROR_OUT_OF_POOL_MEMORY" : "VK_ERROR_OUT_OF_POOL_MEMORY A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: return !get_extended ? "VK_ERROR_INVALID_EXTERNAL_HANDLE" : "VK_ERROR_INVALID_EXTERNAL_HANDLE An external handle is not a valid handle of the specified type.";
        case VK_ERROR_FRAGMENTATION: return !get_extended ? "VK_ERROR_FRAGMENTATION" : "VK_ERROR_FRAGMENTATION A descriptor pool creation has failed due to fragmentation.";
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: return !get_extended ? "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" : "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT A buffer creation failed because the requested address is not available.";
            // NOTE: Same as above
            // case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            //    return !get_extended ? "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" :"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid.";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return !get_extended ? "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" : "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.";
        case VK_ERROR_UNKNOWN: return !get_extended ? "VK_ERROR_UNKNOWN" : "VK_ERROR_UNKNOWN An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
    }
}

bool isVulkanResultSuccess(VkResult result) { // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    // Inticates if the passed result is a success or an error as defined by the Vulkan spec
    // Returns True if success, otherwise false
    // Defaults to true for unknown result types

    switch (result) {
        // Success Codes
        default:
        case VK_SUCCESS:
        case VK_NOT_READY:
        case VK_TIMEOUT:
        case VK_EVENT_SET:
        case VK_EVENT_RESET:
        case VK_INCOMPLETE:
        case VK_SUBOPTIMAL_KHR:
        case VK_THREAD_IDLE_KHR:
        case VK_THREAD_DONE_KHR:
        case VK_OPERATION_DEFERRED_KHR:
        case VK_OPERATION_NOT_DEFERRED_KHR:
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return true;

        // Error codes
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_INITIALIZATION_FAILED:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_MEMORY_MAP_FAILED:
        case VK_ERROR_LAYER_NOT_PRESENT:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_TOO_MANY_OBJECTS:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_INVALID_SHADER_NV:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        case VK_ERROR_FRAGMENTATION:
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT: // NOTE: Same as above
            // case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        case VK_ERROR_UNKNOWN:
            return false;
    }
}