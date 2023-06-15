#pragma once

#include <vulkan/vulkan.h>

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

#define VULKAN_MAX_FRAMES_IN_FLIGHTS 3
#define VULKAN_MAX_PRESENTATION_MODES 8
#define VULKAN_MAX_SURFACE_FORMATS 512
#define VULKAN_MAX_MATERIAL_COUNT 1024
#define VULKAN_MAX_GEOMETRY_COUNT 4096
#define VULKAN_MAX_UI_COUNT 1024
#define VULKAN_SHADER_MAX_STAGES 8
#define VULKAN_SHADER_MAX_GLOBAL_TEXTURES 31
#define VULKAN_SHADER_MAX_INSTANCE_TEXTURES 31
#define VULKAN_SHADER_MAX_ATTRIBUTES 16
#define VULKAN_SHADER_MAX_UNIFORMS 128
#define VULKAN_SHADER_MAX_BINDINGS 2
#define VULKAN_SHADER_MAX_PUSH_CONST_RANGES 32
#define TEXTURE_NAME_MAX_LENGTH 512

//
//typedef enum memory_tag {
//    // For temporary use. Should be assigned one of the below or have a new tag created.
//    MEMORY_TAG_UNKNOWN,
//    MEMORY_TAG_ARRAY,
//    MEMORY_TAG_LINEAR_ALLOCATOR,
//    MEMORY_TAG_DARRAY,
//    MEMORY_TAG_DICT,
//    MEMORY_TAG_RING_QUEUE,
//    MEMORY_TAG_BST,
//    MEMORY_TAG_STRING,
//    MEMORY_TAG_ENGINE,
//    MEMORY_TAG_JOB,
//    MEMORY_TAG_TEXTURE,
//    MEMORY_TAG_MATERIAL_INSTANCE,
//    MEMORY_TAG_RENDERER,
//    MEMORY_TAG_GAME,
//    MEMORY_TAG_TRANSFORM,
//    MEMORY_TAG_ENTITY,
//    MEMORY_TAG_ENTITY_NODE,
//    MEMORY_TAG_SCENE,
//    MEMORY_TAG_RESOURCE,
//    MEMORY_TAG_VULKAN,
//    // "External" vulkan allocations, for reporting purposes only.
//    MEMORY_TAG_VULKAN_EXT,
//    MEMORY_TAG_DIRECT3D,
//    MEMORY_TAG_OPENGL,
//    // Representation of GPU-local/vram
//    MEMORY_TAG_GPU_LOCAL,
//    MEMORY_TAG_BITMAP_FONT,
//    MEMORY_TAG_SYSTEM_FONT,
//    MEMORY_TAG_KEYMAP,
//    MEMORY_TAG_HASHTABLE,
//
//    MEMORY_TAG_MAX_TAGS
//}

enum TextureType {
    TextureType_2D, // @brief A standard two-dimensional texture
    TextureType_CubeMap // A cube texture, used for cubemaps
};

namespace gpu {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSwapchainKHR swapchain;

    VkQueue graphics_queue;
    VkQueue compute_queue;
    VkQueue present_queue;
    VkQueue transfer_queue;

    u32 graphics_queue_family_index = -1;
    u32 compute_queue_family_index = -1;
    u32 present_queue_family_index = -1;
    u32 transfer_queue_family_index = -1;

    VkCommandPool graphics_command_pool;
//        vulkan_command_buffer* graphics_command_buffers; // The graphics command buffers, one per frame


    u32 framebuffer_width = 800;
    u32 framebuffer_height = 600;
    u64 framebuffer_size_generation; // Current generation of framebuffer size. If it does not match framebuffer_size_last_generation, a new one should be generated
    u64 framebuffer_size_last_generation; // The generation of the framebuffer when it was last created. Set to framebuffer_size_generation when updated

    Rect viewport_rect;
    Rect scissor_rect;

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

    namespace _device {
        namespace requirements {
            bool discrete_gpu = true;
            bool graphics = true;
            bool present = true;
            bool compute = true;
            bool transfer = true;
            bool local_host_visible = true;
            bool sampler_anisotropy = false;
        }

        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
        VkPhysicalDeviceFeatures features;

        VkFormat depth_format; // The chosen supported depth format
        u8 depth_channel_count; // The chosen depth format's number of channels

        VkSurfaceKHR surface;
        VkSurfaceCapabilitiesKHR surface_capabilities;
        VkSurfaceFormatKHR surface_formats[VULKAN_MAX_SURFACE_FORMATS];
        unsigned int surface_format_count = 0;

        VkPresentModeKHR present_modes[VULKAN_MAX_PRESENTATION_MODES];
        unsigned int present_mode_count = 0;

        VkExtensionProperties available_extensions[256];
        unsigned int available_extensions_count = 0;

        const char* enabled_extension_names[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        const unsigned int enabled_extensions_count = sizeof(enabled_extension_names) / sizeof(char*);

        i32 getMemoryIndex(
            u32 type_filter, // The memory types to find
            u32 property_flags // Memory properties that need to exist
        );
    }

    enum class State {
        Ready,
        Recording,
        InRenderPass,
        Recorded,
        Submitted,
        UnAllocated
    };

    struct CommandBuffer {
        VkCommandBuffer handle;
        State state;
    };

    CommandBuffer graphics_command_buffers[4];

    struct FrameBuffer {
        VkFramebuffer handle;
    };

    struct RenderTarget {
        struct Attachment {
            enum class Type {
                Color = 1,
                Depth = 2,
                Stencil = 4
            };

            enum class Source {
                Default = 1,
                View = 2
            };

            enum class LoadOp {
                DontCare,
                Load
            };

            enum StoreOp {
                DontCare,
                Store
            };

            struct Config {
                Type type;
                Source source;
                LoadOp load_operation;
                StoreOp store_operation;
                bool present_after;
            };

            Config config;
            struct texture* texture;
        };

        struct Config {
            u8 attachment_count;
            Attachment::Config attachments[4];
        };

        u8 attachment_count;
        Attachment *attachments;
        FrameBuffer *framebuffer;
    };

    struct RenderPass {
        enum class ClearFlags {
            None = 0,
            Color = 1,
            Depth = 2,
            Stencil = 4
        };

        struct Config {
            const char* name;
            RectI render_area;
            Color clear_color;
            f32 depth;
            u32 stencil;
            u8 clear_flags;
            RenderTarget::Config target;
        };

        VkRenderPass handle;
        Config config;
        State state;
        u16 id;
        u8 render_target_count;
        RenderTarget* render_targets;

        bool create(const Config &config);
        void destroy();

        bool begin(RenderTarget &render_target);
        bool end();
    };

    RenderPass main_render_pass;

    struct Image {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
        VkMemoryRequirements memory_requirements;
        VkMemoryPropertyFlags memory_flags;
        u32 width;
        u32 height;
        char* name;

        void create(TextureType type, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, bool create_view, VkImageAspectFlags view_aspect_flags, const char* name);
        void createView(TextureType type, VkFormat format, VkImageAspectFlags aspect_flags);
//        void transitionLayout(TextureType type, CommandBuffer &command_buffer, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
//        void copyFromBuffer(TextureType type, VkBuffer buffer, CommandBuffer &command_buffer);
//        void copyToBuffer(TextureType type, VkBuffer buffer, CommandBuffer &command_buffer);
//        void copyPixelToBuffer(TextureType type, VkBuffer buffer, u32 x, u32 y, CommandBuffer &command_buffer);
        void destroy();
    };

    namespace _swapchain {
        enum SwapchainConfig {
            SwapchainConfig_VSync = 0x1,
            SwapchainConfig_PowerSaving  = 0x2
        };

        VkSurfaceFormatKHR image_format;
        VkPresentModeKHR present_mode;

        // The maximum number of "images in flight" (images simultaneously being rendered to).
        // Typically one less than the total number of images available.
        u8 max_frames_in_flight = 1;

        SwapchainConfig config; // Indicates various flags used for swapchain instantiation
        unsigned int image_count = 0; // The number of swapchain images
        unsigned int image_index = 0;   // The current image index

//        texture* render_textures; // An array of render targets, which contain swapchain images
//        texture* depth_textures; // An array of depth textures, one per frame

        //Render targets used for on-screen rendering, one per frame. The images contained in these are created and owned by the swapchain
//        render_target render_targets[3];
//        render_target world_render_targets[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // Render targets used for world rendering. One per frame

        Image images[VULKAN_MAX_FRAMES_IN_FLIGHTS];
        Image depth_images[VULKAN_MAX_FRAMES_IN_FLIGHTS];
//        VkImage images[VULKAN_MAX_FRAMES_IN_FLIGHTS];
//        VkImageView image_views[VULKAN_MAX_FRAMES_IN_FLIGHTS];

        VkSemaphore image_available_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // The semaphores used to indicate image availability, one per frame
        VkSemaphore queue_complete_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // The semaphores used to indicate queue availability, one per frame
        VkFence images_in_flight[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // Holds pointers to fences which exist and are owned elsewhere, one per frame

        const unsigned int in_flight_fence_count = VULKAN_MAX_FRAMES_IN_FLIGHTS - 1; // The current number of in-flight fences
        VkFence in_flight_fences[VULKAN_MAX_FRAMES_IN_FLIGHTS - 1]; // The in-flight fences, used to indicate to the application when a frame is busy/ready

        u32 current_frame = 0; // The current frame

        bool recreating_swapchain = false; // Indicates if the swapchain is currently being recreated
        bool render_flag_changed = false;
    }

//    renderbuffer object_vertex_buffer; // The object vertex buffer, used to hold geometry vertices
//    renderbuffer object_index_buffer;  // The object index buffer, used to hold geometry indices
//    vulkan_geometry_data geometries[VULKAN_MAX_GEOMETRY_COUNT]; // A collection of loaded geometries

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