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

#define VULKAN_MAX_SWAPCHAIN_FRAME_COUNT 3
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


namespace gpu {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;

    enum VertexAttributeFormat {
        F32,
        F32x2,
        F32x3,
        F32x4,
        I8, U8,
        I16, U16,
        I32, U32
    };

    static VkFormat VERTEX_ATTRIBUTE_FORMATS[11] = {
        VK_FORMAT_R32_SFLOAT,
        VK_FORMAT_R32G32_SFLOAT,
        VK_FORMAT_R32G32B32_SFLOAT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_FORMAT_R8_SINT,
        VK_FORMAT_R8_UINT,
        VK_FORMAT_R16_SINT,
        VK_FORMAT_R16_UINT,
        VK_FORMAT_R32_SINT,
        VK_FORMAT_R32_UINT
    };

    struct VertexAttribute {
        VertexAttributeFormat format;
        u32 size;
    };

    struct VertexDescriptor {
        u32 vertex_input_stride;
        u32 attribute_count;
        VertexAttribute attributes[16];
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

    struct Buffer {
        BufferType type;

        VkBuffer handle;
        VkBufferUsageFlagBits usage;
        bool is_locked;
        VkDeviceMemory memory;
        VkMemoryRequirements memory_requirements;
        i32 memory_index;
        u32 memory_property_flags;

        u64 total_size;
        u64 freelist_memory_requirement;

        INLINE bool isDeviceLocal() const { return (memory_property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; }
        INLINE bool isHostVisible() const { return (memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; }
        INLINE bool isHostCoherent() const { return (memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; }

        bool create(BufferType buffer_type, u64 size);
        void destroy();
        bool resize(u64 new_size);
        bool flush(u64 size = 0, u64 offset = 0) const;
        bool download(void** to, u64 size = 0, u64 from_offset = 0);
        bool upload(const void* from, u64 size = 0, u64 to_offset = 0);
        bool copyTo(const Buffer &to_buffer, u64 size = 0, u64 from_offset = 0, u64 to_offset = 0) const;

    protected:
        void* data(u64 size, u64 offset = 0) const;
        void read(void* to, u64 size, u64 from_offset = 0) const;
        void write(const void* from, u64 size, u64 to_offset = 0);
    };

    struct VertexBuffer : Buffer { bool create(u64 size) { *this = {}; return Buffer::create(BufferType::Vertex, size); } };
    struct IndexBuffer : Buffer {
        VkIndexType index_type = VK_INDEX_TYPE_UINT32;

        bool create(u64 size, VkIndexType type = VK_INDEX_TYPE_UINT32) {
            *this = {};
            bool result = Buffer::create(BufferType::Index, size);
            index_type = type;
            return result;
        }
    };

    enum class State {
        Ready,
        Recording,
        InRenderPass,
        Recorded,
        Submitted,
        UnAllocated
    };

    struct Image {
        VkImage handle;
        VkDeviceMemory memory;
        VkImageView view;
        VkMemoryRequirements memory_requirements;
        VkMemoryPropertyFlags memory_flags;
        u32 width;
        u32 height;
        char* name;
        VkImageViewType type;

        void create(VkImageViewType type, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memory_flags, bool create_view, VkImageAspectFlags view_aspect_flags, const char* name);
        void createView(VkImageViewType type, VkFormat format, VkImageAspectFlags aspect_flags);
        void destroy();
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

    struct FrameBuffer {
        VkFramebuffer handle;

        void create(u32 width, u32 height, VkRenderPass render_pass_handle, VkImageView *image_views, unsigned int image_views_count) {
            VkFramebufferCreateInfo framebuffer_create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer_create_info.renderPass = render_pass_handle;
            framebuffer_create_info.attachmentCount = image_views_count;
            framebuffer_create_info.pAttachments = image_views;
            framebuffer_create_info.width = width;
            framebuffer_create_info.height = height;
            framebuffer_create_info.layers = 1;

            VK_CHECK(vkCreateFramebuffer(device, &framebuffer_create_info, nullptr, &handle))
        }

        void destroy() {
            if (handle == nullptr) return;
            vkDestroyFramebuffer(device, handle, nullptr);
            handle = nullptr;
        }
    };

    struct RenderTarget {
        Attachment::Config attachment_configs[4];
        Attachment *attachments;
        FrameBuffer *framebuffer;
        u8 attachment_count;
    };

    struct RenderPass {
        struct Config {
            const char* name;
            VkRect2D rect;
            Color clear_color;
            f32 depth;
            u32 stencil;
            u8 attachment_count;
            Attachment::Config attachment_configs[4];
        };

        VkRenderPass handle;
        Config config;
        State state;
        u16 id;
//        u8 render_target_count;
//        RenderTarget* render_targets;

        bool create(const Config &config);
        void destroy();
    };

    RenderPass main_render_pass;


    struct Fence {
        VkFence handle = nullptr;
        bool is_signaled = true;

        void create() {
            destroy();

            // Create the fence in a signaled state, indicating that the first frame has already been "rendered".
            // This will prevent the application from waiting indefinitely for the first frame to render since it
            // cannot be rendered until a frame is "rendered" before it.
            VkFenceCreateInfo fence_create_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK(vkCreateFence(device, &fence_create_info, nullptr, &handle))
        }

        void destroy() {
            if (handle) vkDestroyFence(device, handle, nullptr);
            handle = nullptr;
        }
    };

    struct CommandBuffer {
        VkCommandBuffer handle;

    protected:
        const VkCommandPool pool;
        const VkQueue queue;
        const unsigned int queue_family_index;

    public:
        State state = State::Ready;

        explicit CommandBuffer(VkCommandBuffer buffer = nullptr, VkCommandPool pool = nullptr, VkQueue queue = nullptr, unsigned int queue_family_index = 0) : handle{buffer}, pool{pool}, queue{queue}, queue_family_index{queue_family_index} {}
        void begin(bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use);
        void end();
        void beginSingleUse();
        void endSingleUse();
        void reset();
        void free();
        void transitionImageLayout(Image &image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
        void setViewport(VkViewport *viewport);
        void setScissor(VkRect2D *scissor);
    };

    template <typename CommandBufferType>
    struct CommandPool {
        VkCommandPool handle;

        void free(CommandBufferType &command_buffer) {
            vkFreeCommandBuffers(device, handle,1, &command_buffer.handle);
            command_buffer.handle = 0;
            command_buffer.state = State::UnAllocated;
        }

        void destroy() {
            vkDestroyCommandPool(device, handle, nullptr);
        }

        void allocate(CommandBufferType &command_buffer, bool is_primary) {
            if (command_buffer.handle) command_buffer.free();

            VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            allocate_info.commandPool = handle;
            allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            allocate_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer_handle;
            VK_CHECK(vkAllocateCommandBuffers(device, &allocate_info, &command_buffer_handle))
            new(&command_buffer)CommandBufferType(command_buffer_handle, handle);
            command_buffer.reset();
        }

    protected:
        void _create(unsigned int queue_family_index) {
            // Create command pool for graphics queue.
            VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            pool_create_info.queueFamilyIndex = queue_family_index;
            pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            VK_CHECK(vkCreateCommandPool(device, &pool_create_info, nullptr, &handle))
        }
    };

    struct CompiledShader {
        VkShaderStageFlagBits type;
        const char* name;
        const char* entry_point_name;
        uint32_t *code;
        u64 size;
    };

    CompiledShader CreateCompiledShaderFromSourceString(const char* glsl_source, VkShaderStageFlagBits type, const char* name = "shader", const char* entry_point_name = "main");
    CompiledShader CreateCompiledShaderFromSourceFile(const char* source_file, VkShaderStageFlagBits type, const char* name = "shader", const char* entry_point_name = "main");
    CompiledShader CreateCompiledShaderFromBinaryFile(const char* binary_file, VkShaderStageFlagBits type, const char* name = "shader", const char* entry_point_name = "main");

    struct GraphicsPipelineStage {
        VkShaderModule handle;
        VkShaderModuleCreateInfo module_create_info;
        VkPipelineShaderStageCreateInfo create_info;

        void create(const CompiledShader &compiled_shader);
        void destroy();
    };

    struct GraphicsPipeline {
        GraphicsPipelineStage vertex_stage;
        GraphicsPipelineStage fragment_stage;

        VkPipeline handle;
        VkPipelineLayout layout;

        bool create(
            const VertexDescriptor &vertex_descriptor,
            const CompiledShader &vertex_shader_compiled_binary,
            const CompiledShader &fragment_shader_compiled_binary,
            bool is_wireframe = false);

        bool createFromBinaryData(
            const VertexDescriptor &vertex_descriptor,
            uint32_t* vertex_shader_binary_data, size_t vertex_shader_data_size,
            uint32_t* fragment_shader_binary_file, size_t fragment_shader_data_size,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main");

        bool createFromBinaryFiles(
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_binary_file,
            const char* fragment_shader_binary_file,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main");

        bool createFromSourceFiles(
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_source_file,
            const char* fragment_shader_source_file,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main");

        bool createFromSourceStrings(
            const VertexDescriptor &vertex_descriptor,
            const char* vertex_shader_source_string,
            const char* fragment_shader_source_string,
            const char *vertex_shader_name = "vertex_shader",
            const char *fragment_shader_name = "fragment_shader",
            const char *vertex_shader_entry_point_name = "main",
            const char *fragment_shader_entry_point_name = "main");

        void destroy();
    };

    VkQueue graphics_queue;
    unsigned int graphics_queue_family_index;

    struct GraphicsCommandBuffer : CommandBuffer {
        GraphicsCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
            CommandBuffer(command_buffer_handle, command_pool, graphics_queue, graphics_queue_family_index) {}

        bool beginRenderPass(RenderPass &renderpass, VkFramebuffer framebuffer); //, RenderTarget &render_target);
        bool endRenderPass();
        void bind(const GraphicsPipeline &graphics_pipeline) const;
        void bind(const VertexBuffer &vertex_buffer, u32 offset = 0) const;
        void bind(const IndexBuffer &vertex_buffer, u32 offset = 0) const;
        void draw(u32 vertex_count, u32 instance_count = 1, u32 first_vertex = 0, u32 first_instance = 0) const;
    };

    struct GraphicsCommandPool : CommandPool<GraphicsCommandBuffer> {
        void create() {
            _create(graphics_queue_family_index);
            SLIM_LOG_INFO("Graphics command pool created.")
        }
    };

    GraphicsCommandBuffer *graphics_command_buffer = nullptr;
    GraphicsCommandPool graphics_command_pool;

    VkQueue transfer_queue;
    unsigned int transfer_queue_family_index;

    struct TransferCommandBuffer : CommandBuffer {
        TransferCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
            CommandBuffer(command_buffer_handle, command_pool, transfer_queue, transfer_queue_family_index) {}

        void copyBufferToImage(VkBuffer buffer, Image &image);
        void copyImageToBuffer(Image &image, VkBuffer buffer);
        void copyPixelToBuffer(Image &image,  u32 x, u32 y, VkBuffer buffer);
    };

    struct TransferCommandPool : CommandPool<TransferCommandBuffer> {
        void create() {
            _create(transfer_queue_family_index);
            SLIM_LOG_INFO("Transfer command pool created.")
        }
    };

    TransferCommandPool transfer_command_pool;

    VkQueue compute_queue;
    unsigned int compute_queue_family_index;

    struct ComputeCommandBuffer : CommandBuffer {
        ComputeCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
            CommandBuffer(command_buffer_handle, command_pool, compute_queue, compute_queue_family_index) {}
    };

    struct ComputeCommandPool : CommandPool<ComputeCommandBuffer> {
        void create() {
            _create(compute_queue_family_index);
            SLIM_LOG_INFO("Compute command pool created.")
        }
    };

    ComputeCommandPool compute_command_pool;

    namespace present {
        VkViewport main_viewport;
        VkRect2D main_scissor;
        VkSwapchainKHR swapchain;
        VkQueue queue;
        unsigned int queue_family_index;

        struct PresentCommandBuffer : CommandBuffer {
            PresentCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
                CommandBuffer(command_buffer_handle, command_pool, present::queue, present::queue_family_index) {}
        };

        struct PresentCommandPool : CommandPool<PresentCommandBuffer> {
            void create() {
                _create(queue_family_index);
                SLIM_LOG_INFO("Present command pool created.")
            }
        };

        PresentCommandPool command_pool;

        bool vsync = true;
        bool power_saving = false;
        u32 framebuffer_width = DEFAULT_WIDTH;
        u32 framebuffer_height = DEFAULT_HEIGHT;
        u64 framebuffer_size_generation = 0; // Current generation of framebuffer size. If it does not match framebuffer_size_last_generation, a new one should be generated
        u64 framebuffer_size_last_generation = 0; // The generation of the framebuffer when it was last created. Set to framebuffer_size_generation when updated

        VkRect2D viewport_rect;
        VkRect2D scissor_rect;

        u8 depth_channel_count; // The chosen depth format's number of channels
        VkFormat depth_format; // The chosen supported depth format
        VkSurfaceFormatKHR image_format;
        VkPresentModeKHR present_mode;

        // The maximum number of "images in flight" (images simultaneously being rendered to).
        // Typically one less than the total number of images available.
        u8 max_frames_in_flight = VULKAN_MAX_SWAPCHAIN_FRAME_COUNT - 1;
        VkFence in_flight_fences[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT - 1];
        VkFence images_in_flight[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT];

        VkSemaphore image_available_semaphores[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT - 1];
        VkSemaphore queue_complete_semaphores[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT - 1];

        unsigned int image_count = 0; // The number of swapchain images

//        texture* render_textures; // An array of render targets, which contain swapchain images
//        texture* depth_textures; // An array of depth textures, one per frame

        //Render targets used for on-screen rendering, one per frame. The images contained in these are created and owned by the swapchain
//        render_target render_targets[3];
//        render_target world_render_targets[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // Render targets used for world rendering. One per frame

//        const unsigned int in_flight_fence_count = VULKAN_MAX_FRAMES_IN_FLIGHTS - 1; // The current number of in-flight fences
//        VkFence in_flight_fences[VULKAN_MAX_FRAMES_IN_FLIGHTS - 1]; // The in-flight fences, used to indicate to the application when a frame is busy/ready

        u32 current_frame = 0; // The current frame
        bool recreating_swapchain = false; // Indicates if the swapchain is currently being recreated
        bool render_flag_changed = false;

        unsigned int current_image_index = 0;   // The current image index

        struct SwapchainFrame {
            Image image{};
            Image depth_image{};
            FrameBuffer framebuffer{};

            void regenerateFrameBuffer(u32 width, u32 height);
            void create(u32 width, u32 height, VkImage image_handle, u32 index);
            void destroy();
        };

        SwapchainFrame swapchain_frames[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT];
        GraphicsCommandBuffer graphics_command_buffers[VULKAN_MAX_SWAPCHAIN_FRAME_COUNT];

        FrameBuffer *framebuffer = nullptr;
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

        bool present_shares_graphics_queue;
        bool transfer_shares_graphics_queue;

        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
        VkPhysicalDeviceFeatures features;

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