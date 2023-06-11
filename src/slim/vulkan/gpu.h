#pragma once

#include "./core/base.h"

#define VULKAN_MAX_FRAMES_IN_FLIGHTS 3

namespace gpu {
    VkDevice device;

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

    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surface_capabilities;
    VkSurfaceFormatKHR surface_formats[VULKAN_MAX_PRESENTATION_MODES];
    u32 surface_format_count = 0;

    VkPresentModeKHR present_modes[VULKAN_MAX_PRESENTATION_MODES];
    u32 present_mode_count = 0;

    VkFormat depth_format; // The chosen supported depth format
    u8 depth_channel_count; // The chosen depth format's number of channels

    u32 framebuffer_width = 800;
    u32 framebuffer_height = 600;
    u64 framebuffer_size_generation; // Current generation of framebuffer size. If it does not match framebuffer_size_last_generation, a new one should be generated
    u64 framebuffer_size_last_generation; // The generation of the framebuffer when it was last created. Set to framebuffer_size_generation when updated

    Rect viewport_rect;
    Rect scissor_rect;

    namespace instance {
        VkInstance handle;

        VkExtensionProperties available_extensions[256];
        unsigned int available_extensions_count = 0;

        const char* enabled_extension_names[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VULKAN_WSI_SURFACE_EXTENSION_NAME,
#ifdef NDEBUG
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
        };
        const unsigned int enabled_extensions_count = sizeof(instance::enabled_extension_names) / sizeof(char*);


        VkLayerProperties available_validation_layers[8];
        unsigned int available_validation_layers_count = 0;

        const char* enabled_validation_layer_names[] = {
#ifdef NDEBUG
            "VK_LAYER_KHRONOS_validation",
//            "VK_LAYER_LUNARG_api_dump",
#endif
        };
        const unsigned int enabled_validation_layers_count = sizeof(instance::enabled_validation_layer_names) / sizeof(char*);

        bool allEnabledExtensionsAreAvailable(VkExtensionProperties *extensions , unsigned int extensions_count) {
            for (unsigned int i = 0; i <  enabled_extensions_count; ++i) {
                bool found = false;
                for (unsigned int j = 0; j < extensions_count; ++j) {
                    if (!areStringsEqual(instance::enabled_extension_names[i], extensions[j].extensionName)) {
                        found = true;
                        SLIM_LOG_INFO("Found validation layer: %s...", enabled_validation_layer_names[i]);
                        break;
                    }
                }

                if (!found) {
                    SLIM_LOG_FATAL("Required validation layer is missing: %s", enabled_validation_layer_names[i]);
                    return false;
                }
            }

            return true;
        }

        bool init(const char* app_name = "SlimVK") {
//            enabled_extension_count = sizeof(instance::enabled_extension_names) / sizeof(char*);

            VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
            app_info.apiVersion = VK_API_VERSION_1_2;
            app_info.pApplicationName = app_name;
            app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            app_info.pEngineName = "SlimEngine";
            app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

            VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
            create_info.pApplicationInfo = &app_info;
            create_info.enabledExtensionCount = enabled_extensions_count;
            create_info.ppEnabledExtensionNames = instance::enabled_extension_names;

            available_extensions_count = 0;
            vkEnumerateInstanceExtensionProperties(0, &available_extensions_count, 0);
            vkEnumerateInstanceExtensionProperties(0, &available_extensions_count, available_extensions);

            if (!allEnabledExtensionsAreAvailable(available_extensions, available_extensions_count)) return false;

// If validation should be done, get a list of the required validation layert names
// and make sure they exist. Validation layers should only be enabled on non-release builds.
#ifndef NDEBUG
            SLIM_LOG_INFO("Checking validation layers...");

//            enabled_validation_layers_count = sizeof(instance::enabled_validation_layer_names) / sizeof(char*);

            available_validation_layers_count = 0;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&available_validation_layers_count, 0));
            VK_CHECK(vkEnumerateInstanceLayerProperties(&available_validation_layers_count, available_validation_layers));

            // Verify all required layers are available.
            for (u32 i = 0; i < enabled_validation_layers_count; ++i) {
                bool found = false;
                for (u32 j = 0; j < available_validation_layers_count; ++j) {
                    if (areStringsEqual(enabled_validation_layer_names[i], available_validation_layers[j].layerName)) {
                        found = true;
                        SLIM_LOG_INFO("Validation layer found: %s", enabled_validation_layer_names[i]);
                    }
                }

                if (!found) {
                    SLIM_LOG_FATAL("Missing validation layer: %s", enabled_validation_layer_names[i]);
                    return false;
                }
            }

            SLIM_LOG_INFO("Found all enabled validation layers");
#endif
            create_info.enabledLayerCount = enabled_validation_layers_count;
            create_info.ppEnabledLayerNames = enabled_validation_layer_names;

            // #if 'ON_APPLE'
            //create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

            VkResult instance_result = vkCreateInstance(&create_info, 0, &handle);
            if (!vulkan_result_is_success(instance_result)) {
                const char* result_string = vulkan_result_string(instance_result, true);
                SLIM_LOG_FATAL("Vulkan instance creation failed with result: '%s'", result_string);
                return false;
            }

            SLIM_LOG_INFO("Vulkan Instance created");
        }
    }

    namespace physical_device {
        namespace requirements {
            bool discrete_gpu = true;
            bool graphics = true;
            bool present = true;
            bool compute = true;
            bool transfer = true;
            bool local_host_visible = true;
            bool sampler_anisotropy = false;
        }

        VkPhysicalDevice handle{};
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceMemoryProperties memory_properties;
        VkPhysicalDeviceFeatures features;

        VkExtensionProperties available_extensions[256];
        u32 available_extensions_count = 0;

        bool createLogicalDevice() {
            SLIM_LOG_INFO("Creating logical device...");
            // NOTE: Do not create additional queues for shared indices.
            bool present_shares_graphics_queue = graphics_queue_family_index == present_queue_family_index;
            bool transfer_shares_graphics_queue = graphics_queue_family_index == transfer_queue_family_index;

            u32 index_count = 1;
            if (!present_shares_graphics_queue) index_count++;
            if (!transfer_shares_graphics_queue) index_count++;

            u32 indices[32];
            u8 index = 0;
            indices[index++] = graphics_queue_family_index;
            if (!present_shares_graphics_queue) indices[index++] = present_queue_family_index;
            if (!transfer_shares_graphics_queue) indices[index++] = transfer_queue_family_index;

            VkDeviceQueueCreateInfo queue_create_infos[32];
            f32 queue_priority = 1.0f;
            for (u32 i = 0; i < index_count; ++i) {
                queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_infos[i].queueFamilyIndex = indices[i];
                queue_create_infos[i].queueCount = 1;
                queue_create_infos[i].flags = 0;
                queue_create_infos[i].pNext = 0;
                queue_create_infos[i].pQueuePriorities = &queue_priority;
            }

            // Request device features.
            VkPhysicalDeviceFeatures device_features = {};
            device_features.samplerAnisotropy = VK_TRUE;  // Request anistrophy

            bool portability_required = false;
            if (physical_device::available_extensions_count) {
                for (u32 i = 0; i < physical_device::available_extensions_count; ++i) {
                    if (areStringsEqual(physical_device::available_extensions[i].extensionName, "VK_KHR_portability_subset")) {
                        SLIM_LOG_INFO("Adding required extension 'VK_KHR_portability_subset'.");
                        portability_required = true;
                        break;
                    }
                }
            }

            u32 extension_count = portability_required ? 2 : 1;
            const char** extension_names = portability_required
                                           ? (const char* [2]){VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"}
                                           : (const char* [1]){VK_KHR_SWAPCHAIN_EXTENSION_NAME};
            VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            device_create_info.queueCreateInfoCount = index_count;
            device_create_info.pQueueCreateInfos = queue_create_infos;
            device_create_info.pEnabledFeatures = &device_features;
            device_create_info.enabledExtensionCount = extension_count;
            device_create_info.ppEnabledExtensionNames = extension_names;

            // Deprecated and ignored, so pass nothing.
            device_create_info.enabledLayerCount = 0;
            device_create_info.ppEnabledLayerNames = 0;

            // Create the device.
            VK_CHECK(vkCreateDevice(physical_device::handle, &device_create_info, 0, &device));
            VK_SET_DEBUG_OBJECT_NAME(VK_OBJECT_TYPE_DEVICE, &handle, "Vulkan Logical Device");
            SLIM_LOG_INFO("Logical device created");

            // Get queues.
            vkGetDeviceQueue(device, graphics_queue_family_index, 0, &graphics_queue);
            vkGetDeviceQueue(device, present_queue_family_index, 0, &present_queue);
            vkGetDeviceQueue(device, transfer_queue_family_index, 0, &transfer_queue);
            SLIM_LOG_INFO("Queues obtained");

            // Create command pool for graphics queue.
            VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            pool_create_info.queueFamilyIndex = graphics_queue_family_index;
            pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            VK_CHECK(vkCreateCommandPool(device, &pool_create_info, 0, &graphics_command_pool));
            SLIM_LOG_INFO("Graphics command pool created.");

            return true;
        }

        i32 getMemoryIndex(
            u32 type_filter, // The memory types to find
            u32 property_flags // Memory properties that need to exist
        ) {
            // Gets a memory index for the given type and memory properties
            // Returns the index of the found memory type (-1 if not found)

            u32 memory_type_bit = 1;
            for (u32 m = 0; m < memory_properties.memoryTypeCount; m++, memory_type_bit <<= 1) {
                // Check each memory type to see if its bit is set to 1.
                if (type_filter & memory_type_bit &&
                    property_flags == (memory_properties.memoryTypes[m].propertyFlags & property_flags)) {
                    return m;
                }
            }

            SLIM_LOG_WARNING("No suitable memory type found!");
            return -1;
        }

        bool currentPhysicalDeviceMeetsTheRequirements() {
            graphics_queue_family_index = -1;
            present_queue_family_index = -1;
            compute_queue_family_index = -1;
            transfer_queue_family_index = -1;

            if (requirements::discrete_gpu) {
                if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                    SLIM_LOG_INFO("Discrete GPU is required - skipping this device");
                    return false;
                }
            }

            VkQueueFamilyProperties queue_families[8];
            u32 queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_count, 0);
            vkGetPhysicalDeviceQueueFamilyProperties(handle, &queue_family_count, queue_families);

            // Check queue families for their queues support:
            unsigned char min_transfer_score = 255;
            for (unsigned int i = 0; i < queue_family_count; ++i) {
                unsigned char current_transfer_score = 0;

                // Check if the current physical device supports a graphics queue:
                if (graphics_queue_family_index == -1 && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphics_queue_family_index = i;
                    current_transfer_score++;

                    // If also a present queue, this prioritizes grouping of the 2.
                    VkBool32 supports_present = VK_FALSE;
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(handle, i, surface, &supports_present));
                    if (supports_present) {
                        present_queue_family_index = i;
                        current_transfer_score++;
                    }
                }

                // Check if the current physical device supports a compute queue:
                if (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    compute_queue_family_index = i;
                    current_transfer_score++;
                }

                // Check if the current physical device supports a compute queue:
                if (queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    // Take the index if it is the current lowest.
                    // This increases the chance that it is a dedicated transfer queue.
                    if (current_transfer_score <= min_transfer_score) {
                        min_transfer_score = current_transfer_score;
                        transfer_queue_family_index = i;
                    }
                }
            }

            // If a present queue hasn't been found, iterate again and take the first one
            // This should only happen if there is a queue that supports graphics but NOT present
            if (present_queue_family_index == -1) {
                for (unsigned int i = 0; i < queue_family_count; ++i) {
                    VkBool32 supports_present = VK_FALSE;
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(handle, i, surface, &supports_present));
                    if (supports_present) {
                        present_queue_family_index = i;

                        // For troubleshooting purposes, check and log about queue sharing:
                        if (present_queue_family_index != graphics_queue_family_index)
                            SLIM_LOG_WARNING("Warning: Different queue index used for present vs graphics: %u.", i);

                        break;
                    }
                }
            }

            // Print out some info about the device
            SLIM_LOG_INFO("| Graphics | Presentation | Compute | Transfer | Name");
            SLIM_LOG_INFO("|       %d |           %d |      %d |       %d | $s",
                          graphics_queue_family_index != -1,
                          present_queue_family_index != -1,
                          compute_queue_family_index != -1,
                          transfer_queue_family_index != -1,
                          properties.deviceName);

            if ((!requirements::graphics || (requirements::graphics && graphics_queue_family_index != -1)) &&
                (!requirements::present || (requirements::present && present_queue_family_index != -1)) &&
                (!requirements::compute || (requirements::compute && compute_queue_family_index != -1)) &&
                (!requirements::transfer || (requirements::transfer && transfer_queue_family_index != -1))) {
                SLIM_LOG_INFO("Current device is meeting the queue requirements");
                SLIM_LOG_TRACE("Graphics Queue Family Index: %i", graphics_queue_family_index);
                SLIM_LOG_TRACE("Present Queue Family Index:  %i", present_queue_family_index);
                SLIM_LOG_TRACE("Transfer Queue Family Index: %i", transfer_queue_family_index);
                SLIM_LOG_TRACE("Compute Queue Family Index:  %i", compute_queue_family_index);

                VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(handle, surface, &surface_capabilities));
                VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(handle, surface, &surface_format_count, 0));
                if (surface_format_count) {
                    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(handle, surface, &surface_format_count, surface_formats));


                } else {
                    SLIM_LOG_INFO("No surface formats - skipping this device");
                    return false;
                }

                // Present modes
                VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(handle, surface, &present_mode_count, 0));
                if (present_mode_count)
                    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(handle, surface, &present_mode_count, present_modes));

                if (surface_format_count < 1 ||
                    present_mode_count < 1) {
                    SLIM_LOG_INFO("Swap chain unsupported - skipping this device");
                    return false;
                }

                // Format candidates
                const u64 depth_format_candidates_count = 3;
                VkFormat candidates[3] = {
                    VK_FORMAT_D32_SFLOAT,
                    VK_FORMAT_D32_SFLOAT_S8_UINT,
                    VK_FORMAT_D24_UNORM_S8_UINT
                };
                u8 sizes[3] = {4, 4, 3};

                u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
                bool found_depth_format = false;
                for (u64 i = 0; i < depth_format_candidates_count; ++i) {
                    VkFormatProperties format_properties;
                    vkGetPhysicalDeviceFormatProperties(handle, candidates[i], &format_properties);

                    if ((format_properties.linearTilingFeatures & flags) == flags) {
                        depth_format = candidates[i];
                        depth_channel_count = sizes[i];
                        found_depth_format = true;
                        break;
                    } else if ((format_properties.optimalTilingFeatures & flags) == flags) {
                        depth_format = candidates[i];
                        depth_channel_count = sizes[i];
                        found_depth_format = true;
                        break;
                    }
                }

                if (found_depth_format) {
                    depth_format = VK_FORMAT_UNDEFINED;
                    SLIM_LOG_FATAL("Failed to find a supported depth format - skipping this device");
                }

                available_extensions_count = 0;
                VK_CHECK(vkEnumerateDeviceExtensionProperties(handle, 0, &available_extensions_count,0));
                if (!available_extensions_count) return false;

                VK_CHECK(vkEnumerateDeviceExtensionProperties(handle, 0, &available_extensions_count, available_extensions));
                if (!instance::allEnabledExtensionsAreAvailable(available_extensions,available_extensions_count)) {
                    SLIM_LOG_INFO("Some enabled extensions are unavailable - skipping this device");
                    return false;
                }

                if (requirements::sampler_anisotropy && !features.samplerAnisotropy) {
                    SLIM_LOG_INFO("Anisotropic samplers unsupported - skipping this device");
                    return false;
                }

                return true; // This device meets all the requirements
            }

            return false;
        }

        bool init() {
            unsigned int physical_device_count = 0;
            VK_CHECK(vkEnumeratePhysicalDevices(instance::handle, &physical_device_count, 0));

            if (physical_device_count == 0) {
                SLIM_LOG_FATAL("No devices which support Vulkan were found.");
                return false;
            }

            VkPhysicalDevice physical_devices[32];
            VK_CHECK(vkEnumeratePhysicalDevices(instance::handle, &physical_device_count, physical_devices));
            for (unsigned int i = 0; i < physical_device_count; ++i) {
                handle = physical_devices[i];
                vkGetPhysicalDeviceProperties(handle, &properties);
                vkGetPhysicalDeviceFeatures(handle, &features);
                vkGetPhysicalDeviceMemoryProperties(handle, &memory_properties);

                //SLIM_LOG_INFO("Evaluating device: '%s', index %u.", properties.deviceName, i);
                // Check if device supports local/host visible combo
                requirements::local_host_visible = false;
                for (unsigned int m = 0; m < memory_properties.memoryTypeCount; ++i) {
                    // Check each memory type to see if its bit is set to 1.
                    if (((memory_properties.memoryTypes[m].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                        ((memory_properties.memoryTypes[m].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)) {
                        requirements::local_host_visible = true;
                        break;
                    }
                }

                bool result = currentPhysicalDeviceMeetsTheRequirements();
                if (result) {
                    SLIM_LOG_INFO("Selected device: '%s'.", properties.deviceName);
                    switch (properties.deviceType) {
                        default:
                        case VK_PHYSICAL_DEVICE_TYPE_OTHER: SLIM_LOG_INFO("Unknown GPU type"); break;
                        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: SLIM_LOG_INFO("Integrated GPU"); break;
                        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: SLIM_LOG_INFO("Discrete GPU"); break;
                        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: SLIM_LOG_INFO("Virtual GPU"); break;
                        case VK_PHYSICAL_DEVICE_TYPE_CPU: SLIM_LOG_INFO("GPU is CPU"); break;
                    }
                    SLIM_LOG_INFO("GPU Driver version: %d.%d.%d",
                                  VK_VERSION_MAJOR(properties.driverVersion),
                                  VK_VERSION_MINOR(properties.driverVersion),
                                  VK_VERSION_PATCH(properties.driverVersion));

                    SLIM_LOG_INFO("Vulkan API version: %d.%d.%d",
                                  VK_VERSION_MAJOR(properties.apiVersion),
                                  VK_VERSION_MINOR(properties.apiVersion),
                                  VK_VERSION_PATCH(properties.apiVersion));

                    for (unsigned j = 0; j < memory_properties.memoryHeapCount; ++j) {
                        float size = ((float)memory_properties.memoryHeaps[j].size);
                        size /= 1024.0f;
                        size /= 1024.0f;
                        size /= 1024.0f;

                        if (memory_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                            SLIM_LOG_INFO("Local GPU memory (GB): %.2f", size);
                        else
                            SLIM_LOG_INFO("Shared System memory (GB): %.2f", size);
                    }

                    break;
                }
            }

            if (!handle) {
                SLIM_LOG_ERROR("No suitable physical devices found");
                return false;
            }

            SLIM_LOG_INFO("Found a suitable physical device");

            return true;
        }

        i32 findMemoryIndex(u32 type_filter, u32 property_flags) {
            for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
                // Check each memory type to see if its bit is set to 1.
                if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
                    return i;
                }
            }

            SLIM_LOG_WARNING("Unable to find suitable memory type!");
            return -1;
        }
    }

    namespace swap_chain {
        VkSurfaceFormatKHR image_format;

        // The maximum number of "images in flight" (images simultaneously being rendered to).
        // Typically one less than the total number of images available.
        u8 max_frames_in_flight;

        /** @brief Indicates various flags used for swapchain instantiation. */
        renderer_config_flags flags;

        /** @brief The swapchain internal handle. */
        VkSwapchainKHR handle;
        /** @brief The number of swapchain images. */

        u32 image_count;

        /** @brief An array of render targets, which contain swapchain images. */
        texture* render_textures;
        texture* depth_textures; // An array of depth textures, one per frame

        //Render targets used for on-screen rendering, one per frame. The images contained in these are created and owned by the swapchain
//        render_target render_targets[3];
//        render_target world_render_targets[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // Render targets used for world rendering. One per frame

        VkSemaphore image_available_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // The semaphores used to indicate image availability, one per frame
        VkSemaphore queue_complete_semaphores[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // The semaphores used to indicate queue availability, one per frame
        VkFence images_in_flight[VULKAN_MAX_FRAMES_IN_FLIGHTS]; // Holds pointers to fences which exist and are owned elsewhere, one per frame

        const u32 in_flight_fence_count = VULKAN_MAX_FRAMES_IN_FLIGHTS - 1; // The current number of in-flight fences
        VkFence in_flight_fences[VULKAN_MAX_FRAMES_IN_FLIGHTS - 1]; // The in-flight fences, used to indicate to the application when a frame is busy/ready

        u32 image_index;   // The current image index
        u32 current_frame; // The current frame

        bool recreating_swapchain; // Indicates if the swapchain is currently being recreated
        bool render_flag_changed;

        void create(u32 width, u32 height) {
            VkExtent2D swapchain_extent = {width, height};

            // Choose a swap surface format.
            bool found = false;
            for (u32 i = 0; i < surface_format_count; ++i) {
                VkSurfaceFormatKHR format = surface_formats[i];
                // Preferred formats
                if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    image_format = format;
                    found = true;
                    break;
                }
            }

            if (!found) image_format = surface_formats[0];

            // FIFO and MAILBOX support vsync, IMMEDIATE does not.
            // TODO: vsync seems to hold up the game update for some reason.
            // It theoretically should be post-update and pre-render where that happens.
            swapchain->flags = flags;

            VkPresentModeKHR present_mode;
            if (flags & RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT) {
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                // Only try for mailbox mode if not in power-saving mode.
                if ((flags & RENDERER_CONFIG_FLAG_POWER_SAVING_BIT) == 0) {
                    for (u32 i = 0; i < present_mode_count; ++i) {
                        VkPresentModeKHR mode = present_modes[i];
                        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                            present_mode = mode;
                            break;
                        }
                    }
                }
            } else
                present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

            // Swapchain extent
            if (surface_capabilities.currentExtent.width != UINT32_MAX)
                swapchain_extent = surface_capabilities.currentExtent;

            // Clamp to the value allowed by the GPU.
            VkExtent2D min = surface_capabilities.minImageExtent;
            VkExtent2D max = surface_capabilities.maxImageExtent;
            swapchain_extent.width = clampedValue(swapchain_extent.width, min.width, max.width);
            swapchain_extent.height = clampedValue(swapchain_extent.height, min.height, max.height);

            u32 image_count = surface_capabilities.minImageCount + 1;
            if (surface_capabilities.maxImageCount > 0 &&
                image_count > surface_capabilities.maxImageCount) {
                image_count = surface_capabilities.maxImageCount;
            }

            max_frames_in_flight = image_count - 1;

            // Swapchain create info
            VkSwapchainCreateInfoKHR swapchain_create_info = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
            swapchain_create_info.surface = surface;
            swapchain_create_info.minImageCount = image_count;
            swapchain_create_info.imageFormat = image_format.format;
            swapchain_create_info.imageColorSpace = image_format.colorSpace;
            swapchain_create_info.imageExtent = swapchain_extent;
            swapchain_create_info.imageArrayLayers = 1;
            swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            // Setup the queue family indices
            if (graphics_queue_family_index != present_queue_family_index) {
                const unsigned int queueFamilyIndices[] = {
                    graphics_queue_family_index,
                    present_queue_family_index
                };
                swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchain_create_info.queueFamilyIndexCount = 2;
                swapchain_create_info.pQueueFamilyIndices = queueFamilyIndices;
            } else {
                swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                swapchain_create_info.queueFamilyIndexCount = 0;
                swapchain_create_info.pQueueFamilyIndices = 0;
            }

            swapchain_create_info.preTransform = surface::capabilities.currentTransform;
            swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchain_create_info.presentMode = present_mode;
            swapchain_create_info.clipped = VK_TRUE;
            swapchain_create_info.oldSwapchain = 0;

            VK_CHECK(vkCreateSwapchainKHR(device, &swapchain_create_info, 0, &handle));

            // Start with a zero frame index.
            current_frame = 0;

            // Images
            image_count = 0;
            VK_CHECK(vkGetSwapchainImagesKHR(device, handle, &image_count, 0));
            if (!render_textures) {
                render_textures = (texture*)kallocate(sizeof(texture) * swapchain->image_count, MEMORY_TAG_RENDERER);
                // If creating the array, then the internal texture objects aren't created yet either.
                for (u32 i = 0; i < image_count; ++i) {
                    void* internal_data = kallocate(sizeof(vulkan_image), MEMORY_TAG_TEXTURE);

                    char tex_name[38] = "__internal_vulkan_swap chain_image_0__";
                    tex_name[34] = '0' + (char)i;

                    texture_system_wrap_internal(tex_name, swapchain_extent.width, swapchain_extent.height, 4,
                        false, true, false, internal_data, &render_textures[i]);
                    if (!render_textures[i].internal_data) {
                        SLIM_LOG_FATAL("Failed to generate new swap chain image texture!");
                        return;
                    }
                }
            } else {
                for (u32 i = 0; i < image_count; ++i) {
                    // Just update the dimensions.
                    texture_system_resize(&render_textures[i], swapchain_extent.width, swapchain_extent.height, false);
                }
            }
            VkImage swapchain_images[32];
            VK_CHECK(vkGetSwapchainImagesKHR(device, handle, &image_count, swapchain_images));
            for (u32 i = 0; i < image_count; ++i) {
                // Update the internal image for each.
                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;
                image->handle = swapchain_images[i];
                image->width = swapchain_extent.width;
                image->height = swapchain_extent.height;
            }

            // Views
            for (u32 i = 0; i < image_count; ++i) {
                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;

                VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
                view_info.image = image->handle;
                view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                view_info.format = image_format.format;
                view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                view_info.subresourceRange.baseMipLevel = 0;
                view_info.subresourceRange.levelCount = 1;
                view_info.subresourceRange.baseArrayLayer = 0;
                view_info.subresourceRange.layerCount = 1;

                VK_CHECK(vkCreateImageView(device, &view_info, 0, &image->view));
            }

            if (!depth_textures) {
                depth_textures = (texture*)kallocate(sizeof(texture) * image_count, MEMORY_TAG_RENDERER);
            }

            for (u32 i = 0; i < image_count; ++i) {
                // Create depth image and its view.
                char formatted_name[TEXTURE_NAME_MAX_LENGTH] = {0};
                string_format(formatted_name, "swapchain_image_%u", i);

                vulkan_image* image = kallocate(sizeof(vulkan_image), MEMORY_TAG_TEXTURE);
                vulkan_image_create(
                    TEXTURE_TYPE_2D,
                    swapchain_extent.width,
                    swapchain_extent.height,
                    device.depth_format,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    true,
                    VK_IMAGE_ASPECT_DEPTH_BIT,
                    formatted_name,
                    image);

                // Wrap it in a texture.
                texture_system_wrap_internal(
                    "__kohi_default_depth_texture__",
                    swapchain_extent.width,
                    swapchain_extent.height,
                    depth_channel_count,
                    false,
                    true,
                    false,
                    image,
                    &depth_textures[i]);
            }

            SLIM_LOG_INFO("Swap chain created");
        }

        void destroy() {
            vkDeviceWaitIdle(device);

            for (u32 i = 0; i < image_count; ++i) {
                vulkan_image_destroy((vulkan_image*)swapchain->depth_textures[i].internal_data);
                depth_textures[i].internal_data = 0;
            }

            // Only destroy the views, not the images, since those are owned by the swapchain and are thus
            // destroyed when it is.
            for (u32 i = 0; i < image_count; ++i) {
                vulkan_image* image = (vulkan_image*)swapchain->render_textures[i].internal_data;
                vkDestroyImageView(device, image->view, 0);
            }

            vkDestroySwapchainKHR(device, handle, 0);
        }

        void recreate(u32 width, u32 height) {
            destroy();
            create(width, height);
        }

        bool acquireNextImageIndex(u64 timeout_ns, VkSemaphore image_available_semaphore, VkFence fence, u32* out_image_index) {
            VkResult result = vkAcquireNextImageKHR(device, handle, timeout_ns, image_available_semaphore, fence, out_image_index);
            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                // Trigger swapchain recreation, then boot out of the render loop.
                recreate(framebuffer_width, framebuffer_height);

                return false;
            } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                SLIM_LOG_FATAL("Failed to acquire swapchain image!");
                return false;
            }

            return true;
        }

        void present(VkSemaphore render_complete_semaphore, u32 present_image_index) {
            // Return the image to the swapchain for presentation.
            VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
            present_info.waitSemaphoreCount = 1;
            present_info.pWaitSemaphores = &render_complete_semaphore;
            present_info.swapchainCount = 1;
            present_info.pSwapchains = &handle;
            present_info.pImageIndices = &present_image_index;
            present_info.pResults = 0;

            VkResult result = vkQueuePresentKHR(present_queue, &present_info);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
                recreate(framebuffer_width, framebuffer_height);
                SLIM_LOG_DEBUG("Swapchain recreated because swapchain returned out of date or suboptimal.");
            } else if (result != VK_SUCCESS) {
                SLIM_LOG_FATAL("Failed to present swap chain image!");
            }

            // Increment (and loop) the index.
            current_frame = (current_frame + 1) % max_frames_in_flight;
        }
    }

#ifndef NDEBUG
    namespace debug {
        VkDebugUtilsMessengerEXT messenger; // The debug messenger, if active
        PFN_vkSetDebugUtilsObjectNameEXT pfnSetObjectNameEXT; // The function pointer to set debug object names
        PFN_vkSetDebugUtilsObjectTagEXT pfnSetObjectTagEXT; // The function pointer to set free-form debug object tag data
        PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginLabelEXT;
        PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndLabelEXT;

        bool setObjectName(VkObjectType object_type, void* object_handle, const char* object_name) {
            const VkDebugUtilsObjectNameInfoEXT name_info = {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                0,
                object_type,
                (u64)object_handle,
                object_name,
            };

            if (pfnSetObjectNameEXT) {
                VK_CHECK(pfnSetObjectNameEXT(device, &name_info));
                return true;
            }

            return false;
        }

        bool setObjectTag(VkObjectType object_type, void* object_handle, u64 tag_size, const void* tag_data) {
            const VkDebugUtilsObjectTagInfoEXT tag_info = {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                0,
                object_type,
                (uint64_t)object_handle,
                0,
                tag_size,
                tag_data};

            if (pfnSetObjectTagEXT) {
                VK_CHECK(pfnSetObjectTagEXT(device, &tag_info));
                return true;
            }

            return false;
        }

        bool beginLabel(VkCommandBuffer buffer, const char* label_name, Color color) {
            VkDebugUtilsLabelEXT label_info = {
                VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
                0,
                label_name};

            label_info.color[0] = color.r;
            label_info.color[1] = color.g;
            label_info.color[2] = color.b;
            label_info.color[3] = 1.0f;

            if (pfnCmdBeginLabelEXT) {
                pfnCmdBeginLabelEXT(buffer, &label_info);
                return true;
            }
            return false;
        }

        bool endLabel(VkCommandBuffer buffer) {
            if (pfnCmdEndLabelEXT) {
                pfnCmdEndLabelEXT(buffer);
                return true;
            }
            return false;
        }

        bool init() {
            SLIM_LOG_DEBUG("Creating Vulkan debugger...");
            u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;  //|
            //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

            VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
            debug_create_info.messageSeverity = log_severity;
            debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debug_create_info.pfnUserCallback = vk_debug_callback;

            PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance::handle, "vkCreateDebugUtilsMessengerEXT");
            //    KASSERT_MSG(func, "Failed to create debug messenger!");
            VK_CHECK(func(instance::handle, &debug_create_info, 0, &messenger));
            SLIM_LOG_DEBUG("Vulkan debugger created.");

            // Load up debug function pointers.
            pfnSetObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance::handle, "vkSetDebugUtilsObjectNameEXT");
            if (!pfnSetObjectNameEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkSetDebugUtilsObjectNameEXT. Debug functions associated with this will not work.");
            }
            pfnSetObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(instance::handle, "vkSetDebugUtilsObjectTagEXT");
            if (!pfnSetObjectTagEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkSetDebugUtilsObjectTagEXT. Debug functions associated with this will not work.");
            }

            pfnCmdBeginLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance::handle, "vkCmdBeginDebugUtilsLabelEXT");
            if (!pfnCmdBeginLabelEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkCmdBeginDebugUtilsLabelEXT. Debug functions associated with this will not work.");
            }

            pfnCmdEndLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance::handle, "vkCmdEndDebugUtilsLabelEXT");
            if (!pfnCmdEndLabelEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkCmdEndDebugUtilsLabelEXT. Debug functions associated with this will not work.");
            }
        }
    }
#endif

//    renderbuffer object_vertex_buffer; // The object vertex buffer, used to hold geometry vertices
//    renderbuffer object_index_buffer;  // The object index buffer, used to hold geometry indices
//    vulkan_geometry_data geometries[VULKAN_MAX_GEOMETRY_COUNT]; // A collection of loaded geometries

    bool init(const char* app_name = "SlimVK") {
#ifndef NDEBUG
        debug::init();
#endif

        instance::init(app_name);

        SLIM_LOG_DEBUG("Creating Vulkan surface...");
        if (!CreateVulkanSurface(instance::handle, surface)) {
            SLIM_LOG_ERROR("Failed to create platform surface!");
            return false;
        }
        SLIM_LOG_DEBUG("Vulkan surface created.");

        if (!physical_device::init()) {
            // TODO Report..
            return false;
        }

        // Device creation
        if (!logical_device::create()) {
            SLIM_LOG_ERROR("Failed to create device!");
            return false;
        }

        // Create buffers

//        // Geometry vertex buffer
//        const u64 vertex_buffer_size = sizeof(vertex_3d) * 1024 * 1024;
//        if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_VERTEX, vertex_buffer_size, true, &context->object_vertex_buffer)) {
//            KERROR("Error creating vertex buffer.");
//            return false;
//        }
//        renderer_renderbuffer_bind(&context->object_vertex_buffer, 0);
//
//        // Geometry index buffer
//        const u64 index_buffer_size = sizeof(u32) * 1024 * 1024;
//        if (!renderer_renderbuffer_create(RENDERBUFFER_TYPE_INDEX, index_buffer_size, true, &context->object_index_buffer)) {
//            KERROR("Error creating index buffer.");
//            return false;
//        }
//        renderer_renderbuffer_bind(&context->object_index_buffer, 0);
//
//        // Mark all geometries as invalid
//        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
//            context->geometries[i].id = INVALID_ID;
//        }
//
//        SLIM_LOG_INFO("Vulkan renderer initialized successfully.");
//        return true;
    }
}


/**
 * @brief Represents a Vulkan-specific buffer.
 * Used to load data onto the GPU.
 */
typedef struct vulkan_buffer {
    /** @brief The handle to the internal buffer. */
    VkBuffer handle;
    /** @brief The usage flags. */
    VkBufferUsageFlagBits usage;
    /** @brief Indicates if the buffer's memory is currently locked. */
    b8 is_locked;
    /** @brief The memory used by the buffer. */
    VkDeviceMemory memory;
    /** @brief The memory requirements for this buffer. */
    VkMemoryRequirements memory_requirements;
    /** @brief The index of the memory used by the buffer. */
    i32 memory_index;
    /** @brief The property flags for the memory used by the buffer. */
    u32 memory_property_flags;
} vulkan_buffer;


/**
 * @brief A representation of a Vulkan image. This can be thought
 * of as a texture. Also contains the view and memory used by
 * the internal image.
 */
typedef struct vulkan_image {
    /** @brief The handle to the internal image object. */
    VkImage handle;
    /** @brief The memory used by the image. */
    VkDeviceMemory memory;
    /** @brief The view for the image, which is used to access the image. */
    VkImageView view;
    /** @brief The GPU memory requirements for this image. */
    VkMemoryRequirements memory_requirements;
    /** @brief Memory property flags */
    VkMemoryPropertyFlags memory_flags;
    /** @brief The image width. */
    u32 width;
    /** @brief The image height. */
    u32 height;
    /** @brief The name of the image. */
    char* name;
} vulkan_image;

/** @brief Represents the possible states of a renderpass. */
typedef enum vulkan_render_pass_state {
    /** @brief The renderpass is ready to begin. */
    READY,
    /** @brief The renderpass is currently being recorded to. */
    RECORDING,
    /** @brief The renderpass is currently active. */
    IN_RENDER_PASS,
    /** @brief The renderpass is has ended recording. */
    RECORDING_ENDED,
    /** @brief The renderpass has been submitted to the queue. */
    SUBMITTED,
    /** @brief The renderpass is not allocated. */
    NOT_ALLOCATED
} vulkan_render_pass_state;

/**
 * @brief A representation of the Vulkan renderpass.
 */
typedef struct vulkan_renderpass {
    /** @brief The internal renderpass handle. */
    VkRenderPass handle;
    /** @brief The current render area of the renderpass. */

    /** @brief The depth clear value. */
    f32 depth;
    /** @brief The stencil clear value. */
    u32 stencil;

    /** @brief Indicates renderpass state. */
    vulkan_render_pass_state state;
} vulkan_renderpass;

/**
 * @brief Representation of the Vulkan swapchain.
 */
typedef struct vulkan_swapchain {
    /** @brief The swapchain image format. */
    VkSurfaceFormatKHR image_format;
    /**
     * @brief The maximum number of "images in flight" (images simultaneously being rendered to).
     * Typically one less than the total number of images available.
     */
    u8 max_frames_in_flight;

    /** @brief Indicates various flags used for swapchain instantiation. */
    renderer_config_flags flags;

    /** @brief The swapchain internal handle. */
    VkSwapchainKHR handle;
    /** @brief The number of swapchain images. */
    u32 image_count;
    /** @brief An array of render targets, which contain swapchain images. */
    texture* render_textures;

    /** @brief An array of depth textures, one per frame. */
    texture* depth_textures;

    /**
     * @brief Render targets used for on-screen rendering, one per frame.
     * The images contained in these are created and owned by the swapchain.
     * */
    render_target render_targets[3];
} vulkan_swapchain;

/**
 * @brief Represents all of the available states that
 * a command buffer can be in.
 */
typedef enum vulkan_command_buffer_state {
    /** @brief The command buffer is ready to begin. */
    COMMAND_BUFFER_STATE_READY,
    /** @brief The command buffer is currently being recorded to. */
    COMMAND_BUFFER_STATE_RECORDING,
    /** @brief The command buffer is currently active. */
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    /** @brief The command buffer is has ended recording. */
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    /** @brief The command buffer has been submitted to the queue. */
    COMMAND_BUFFER_STATE_SUBMITTED,
    /** @brief The command buffer is not allocated. */
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} vulkan_command_buffer_state;

/**
 * @brief Represents a Vulkan-specific command buffer, which
 * holds a list of commands and is submitted to a queue
 * for execution.
 */
typedef struct vulkan_command_buffer {
    /** @brief The internal command buffer handle. */
    VkCommandBuffer handle;

    /** @brief Command buffer state. */
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

/**
 * @brief Represents a single shader stage.
 */
typedef struct vulkan_shader_stage {
    /** @brief The shader module creation info. */
    VkShaderModuleCreateInfo create_info;
    /** @brief The internal shader module handle. */
    VkShaderModule handle;
    /** @brief The pipeline shader stage creation info. */
    VkPipelineShaderStageCreateInfo shader_stage_create_info;
} vulkan_shader_stage;

/**
 * @brief A configuration structure for Vulkan pipelines.
 */
typedef struct vulkan_pipeline_config {
    /** @brief A pointer to the renderpass to associate with the pipeline. */
    vulkan_renderpass* renderpass;
    /** @brief The stride of the vertex data to be used (ex: sizeof(vertex_3d)) */
    u32 stride;
    /** @brief The number of attributes. */
    u32 attribute_count;
    /** @brief An array of attributes. */
    VkVertexInputAttributeDescription* attributes;
    /** @brief The number of descriptor set layouts. */
    u32 descriptor_set_layout_count;
    /** @brief An array of descriptor set layouts. */
    VkDescriptorSetLayout* descriptor_set_layouts;
    /** @brief The number of stages (vertex, fragment, etc). */
    u32 stage_count;
    /** @brief An VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BITarray of stages. */
    VkPipelineShaderStageCreateInfo* stages;
    /** @brief The initial viewport configuration. */
    VkViewport viewport;
    /** @brief The initial scissor configuration. */
    VkRect2D scissor;
    /** @brief The face cull mode. */
    face_cull_mode cull_mode;
    /** @brief Indicates if this pipeline should use wireframe mode. */
    b8 is_wireframe;
    /** @brief The shader flags used for creating the pipeline. */
    u32 shader_flags;
    /** @brief The number of push constant data ranges. */
    u32 push_constant_range_count;
    /** @brief An array of push constant data ranges. */
    range* push_constant_ranges;
} vulkan_pipeline_config;

/**
 * @brief Holds a Vulkan pipeline and its layout.
 */
typedef struct vulkan_pipeline {
    /** @brief The internal pipeline handle. */
    VkPipeline handle;
    /** @brief The pipeline layout. */
    VkPipelineLayout pipeline_layout;
} vulkan_pipeline;


/**
 * @brief Internal buffer data for geometry. This data gets loaded
 * directly into a buffer.
 */
typedef struct vulkan_geometry_data {
    /** @brief The unique geometry identifier. */
    u32 id;
    /** @brief The geometry generation. Incremented every time the geometry data changes. */
    u32 generation;
    /** @brief The vertex count. */
    u32 vertex_count;
    /** @brief The size of each vertex. */
    u32 vertex_element_size;
    /** @brief The offset in bytes in the vertex buffer. */
    u64 vertex_buffer_offset;
    /** @brief The index count. */
    u32 index_count;
    /** @brief The size of each index. */
    u32 index_element_size;
    /** @brief The offset in bytes in the index buffer. */
    u64 index_buffer_offset;
} vulkan_geometry_data;



/**
 * @brief Configuration for a shader stage, such as vertex or fragment.
 */
typedef struct vulkan_shader_stage_config {
    /** @brief The shader stage bit flag. */
    VkShaderStageFlagBits stage;
    /** @brief The shader file name. */
    char file_name[255];

} vulkan_shader_stage_config;

/**
 * @brief The configuration for a descriptor set.
 */
typedef struct vulkan_descriptor_set_config {
    /** @brief The number of bindings in this set. */
    u8 binding_count;
    /** @brief An array of binding layouts for this set. */
    VkDescriptorSetLayoutBinding bindings[VULKAN_SHADER_MAX_BINDINGS];
    /** @brief The index of the sampler binding. */
    u8 sampler_binding_index;
} vulkan_descriptor_set_config;

/** @brief Internal shader configuration generated by vulkan_shader_create(). */
typedef struct vulkan_shader_config {
    /** @brief The number of shader stages in this shader. */
    u8 stage_count;
    /** @brief  The configuration for every stage of this shader. */
    vulkan_shader_stage_config stages[VULKAN_SHADER_MAX_STAGES];
    /** @brief An array of descriptor pool sizes. */
    VkDescriptorPoolSize pool_sizes[2];
    /**
     * @brief The max number of descriptor sets that can be allocated from this shader.
     * Should typically be a decently high number.
     */
    u16 max_descriptor_set_count;

    /**
     * @brief The total number of descriptor sets configured for this shader.
     * Is 1 if only using global uniforms/samplers; otherwise 2.
     */
    u8 descriptor_set_count;
    /** @brief Descriptor sets, max of 2. Index 0=global, 1=instance */
    vulkan_descriptor_set_config descriptor_sets[2];

    /** @brief An array of attribute descriptions for this shader. */
    VkVertexInputAttributeDescription attributes[VULKAN_SHADER_MAX_ATTRIBUTES];

    /** @brief Face culling mode, provided by the front end. */
    face_cull_mode cull_mode;

} vulkan_shader_config;

/**
 * @brief Represents a state for a given descriptor. This is used
 * to determine when a descriptor needs updating. There is a state
 * per frame (with a max of 3).
 */
typedef struct vulkan_descriptor_state {
    /** @brief The descriptor generation, per frame. */
    u8 generations[3];
    /** @brief The identifier, per frame. Typically used for texture ids. */
    u32 ids[3];
} vulkan_descriptor_state;

/**
 * @brief Represents the state for a descriptor set. This is used to track
 * generations and updates, potentially for optimization via skipping
 * sets which do not need updating.
 */
typedef struct vulkan_shader_descriptor_set_state {
    /** @brief The descriptor sets for this instance, one per frame. */
    VkDescriptorSet descriptor_sets[3];

    /** @brief A descriptor state per descriptor, which in turn handles frames. Count is managed in shader config. */
    vulkan_descriptor_state descriptor_states[VULKAN_SHADER_MAX_BINDINGS];
} vulkan_shader_descriptor_set_state;

/**
 * @brief The instance-level state for a shader.
 */
typedef struct vulkan_shader_instance_state {
    /** @brief The instance id. INVALID_ID if not used. */
    u32 id;
    /** @brief The offset in bytes in the instance uniform buffer. */
    u64 offset;

    /** @brief  A state for the descriptor set. */
    vulkan_shader_descriptor_set_state descriptor_set_state;

    /**
     * @brief Instance texture map pointers, which are used during rendering. These
     * are set by calls to set_sampler.
     */
    struct texture_map** instance_texture_maps;
} vulkan_shader_instance_state;

/**
 * @brief Represents a generic Vulkan shader. This uses a set of inputs
 * and parameters, as well as the shader programs contained in SPIR-V
 * files to construct a shader for use in rendering.
 */
typedef struct vulkan_shader {
    /** @brief The block of memory mapped to the uniform buffer. */
    void* mapped_uniform_buffer_block;

    /** @brief The shader identifier. */
    u32 id;

    /** @brief The configuration of the shader generated by vulkan_create_shader(). */
    vulkan_shader_config config;

    /** @brief A pointer to the renderpass to be used with this shader. */
    vulkan_renderpass* renderpass;

    /** @brief An array of stages (such as vertex and fragment) for this shader. Count is located in config.*/
    vulkan_shader_stage stages[VULKAN_SHADER_MAX_STAGES];

    /** @brief The descriptor pool used for this shader. */
    VkDescriptorPool descriptor_pool;

    /** @brief Descriptor set layouts, max of 2. Index 0=global, 1=instance. */
    VkDescriptorSetLayout descriptor_set_layouts[2];
    /** @brief Global descriptor sets, one per frame. */
    VkDescriptorSet global_descriptor_sets[3];
    /** @brief The uniform buffer used by this shader. */
    renderbuffer uniform_buffer;

    /** @brief The pipeline associated with this shader. */
    vulkan_pipeline pipeline;

    /** @brief The instance states for all instances. @todo TODO: make dynamic */
    u32 instance_count;
    vulkan_shader_instance_state instance_states[VULKAN_MAX_MATERIAL_COUNT];

    /** @brief The number of global non-sampler uniforms. */
    u8 global_uniform_count;
    /** @brief The number of global sampler uniforms. */
    u8 global_uniform_sampler_count;
    /** @brief The number of instance non-sampler uniforms. */
    u8 instance_uniform_count;
    /** @brief The number of instance sampler uniforms. */
    u8 instance_uniform_sampler_count;
    /** @brief The number of local non-sampler uniforms. */
    u8 local_uniform_count;

} vulkan_shader;



void vulkan_device_destroy(vulkan_context* context) {
    // Unset queues
    context->device.graphics_queue = 0;
    context->device.present_queue = 0;
    context->device.transfer_queue = 0;

    SLIM_LOG_INFO("Destroying command pools...");
    vkDestroyCommandPool(
        &logical_device::handle,
        context->device.graphics_command_pool,
        context->allocator);

    // Destroy logical device
    SLIM_LOG_INFO("Destroying logical device...");
    if (&logical_device::handle) {
        vkDestroyDevice(&logical_device::handle, context->allocator);
        &logical_device::handle = 0;
    }

    // Physical devices are not destroyed.
    SLIM_LOG_INFO("Releasing physical device resources...");
    physical_device::handle = 0;

    if (context->device.swapchain_support.formats) {
        kfree(
            context->device.swapchain_support.formats,
            sizeof(VkSurfaceFormatKHR) * context->device.swapchain_support.format_count,
            MEMORY_TAG_RENDERER);
        context->device.swapchain_support.formats = 0;
        context->device.swapchain_support.format_count = 0;
    }

    if (context->device.swapchain_support.present_modes) {
        kfree(
            context->device.swapchain_support.present_modes,
            sizeof(VkPresentModeKHR) * context->device.swapchain_support.present_mode_count,
            MEMORY_TAG_RENDERER);
        context->device.swapchain_support.present_modes = 0;
        context->device.swapchain_support.present_mode_count = 0;
    }

    kzero_memory(
        &context->device.swapchain_support.capabilities,
        sizeof(context->device.swapchain_support.capabilities));

    context->device.graphics_queue_family_index = -1;
    context->device.present_queue_family_index = -1;
    context->device.transfer_queue_family_index = -1;
}

b8 vulkan_device_detect_depth_format(vulkan_device* device) {
    // Format candidates
    const u64 candidate_count = 3;
    VkFormat candidates[3] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT};

    u8 sizes[3] = {
        4,
        4,
        3};

    u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    for (u64 i = 0; i < candidate_count; ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->physical_device, candidates[i], &properties);

        if ((properties.linearTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            device->depth_channel_count = sizes[i];
            return true;
        } else if ((properties.optimalTilingFeatures & flags) == flags) {
            device->depth_format = candidates[i];
            device->depth_channel_count = sizes[i];
            return true;
        }
    }

    return false;
}
