#pragma once

#include "./base.h"
#include "./debug.h"

namespace gpu {
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

        void querySupportForSurfaceFormatsAndPresentModes() {
            VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities))
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR( physical_device, surface, &surface_format_count, nullptr))
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr))
            if (surface_format_count != 0) VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &surface_format_count, surface_formats))
            if (  present_mode_count != 0) VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes))

            // Choose a swap surface format.
            present::image_format = surface_formats[0];
            for (u32 i = 0; i <  surface_format_count; ++i) {
                VkSurfaceFormatKHR format = surface_formats[i];
                // Preferred formats
                if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    present::image_format = format;
                    break;
                }
            }
        }

        bool querySupportForDepthFormats() {
            const u64 candidate_count = 3;
            VkFormat candidates[3] = {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D24_UNORM_S8_UINT
            };
            u8 sizes[3] = {4, 4, 3};

            u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
            for (u64 i = 0; i < candidate_count; ++i) {
                VkFormatProperties depth_format_properties;
                vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &depth_format_properties);

                if ((depth_format_properties.linearTilingFeatures & flags) == flags) {
                    present::depth_format = candidates[i];
                    present::depth_channel_count = sizes[i];
                    return true;
                } else if ((depth_format_properties.optimalTilingFeatures & flags) == flags) {
                    present::depth_format = candidates[i];
                    present::depth_channel_count = sizes[i];
                    return true;
                }
            }

            return false;
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

            available_extensions_count = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &available_extensions_count, nullptr));

            if (available_extensions_count == 0) {
                SLIM_LOG_FATAL("No device extensions found - skipping this device - skipping this device");
                return false;
            }

            VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &available_extensions_count, available_extensions));
            if (!allEnabledExtensionsAreAvailable(enabled_extension_names, enabled_extensions_count,
                                                  available_extensions, available_extensions_count)) {
                SLIM_LOG_INFO("Some enabled extensions are unavailable - skipping this device");
                return false;
            }

            VkQueueFamilyProperties queue_families[5];
            unsigned int queue_family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

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
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, _device::surface, &supports_present));
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
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, _device::surface, &supports_present));
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

                _device::querySupportForSurfaceFormatsAndPresentModes();
                if (_device::surface_format_count < 1 ||
                    _device::present_mode_count < 1) {
                    SLIM_LOG_INFO("Swap chain unsupported - skipping this device");
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
            VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, 0));

            if (physical_device_count == 0) {
                SLIM_LOG_FATAL("No devices which support Vulkan were found.");
                return false;
            }

            VkPhysicalDevice physical_devices[4];
            VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices));
            for (unsigned int i = 0; i < physical_device_count; ++i) {
                physical_device = physical_devices[i];
                vkGetPhysicalDeviceProperties(physical_device, &properties);
                vkGetPhysicalDeviceFeatures(physical_device, &features);
                vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

                //SLIM_LOG_INFO("Evaluating device: '%s', index %u.", properties.deviceName, i);
                // Check if device supports local/host visible combo
                requirements::local_host_visible = false;
                for (unsigned int m = 0; m < memory_properties.memoryTypeCount; m++) {
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

                        if (memory_properties.memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                            SLIM_LOG_INFO("Local GPU memory (GB): %.2f", size);
                        } else {
                            SLIM_LOG_INFO("Shared System memory (GB): %.2f", size);
                        }
                    }

                    break;
                }
            }

            if (!physical_device) {
                SLIM_LOG_ERROR("No suitable physical devices found");
                return false;
            }

            SLIM_LOG_INFO("Found a suitable physical device");

            SLIM_LOG_INFO("Creating logical device...");
            // NOTE: Do not create additional queues for shared indices.
            present_shares_graphics_queue = graphics_queue_family_index == present_queue_family_index;
            transfer_shares_graphics_queue = graphics_queue_family_index == transfer_queue_family_index;

            u32 index_count = 1;
            if (!present_shares_graphics_queue) index_count++;
            if (!transfer_shares_graphics_queue) index_count++;

            u32 indices[4];
            u8 index = 0;
            indices[index++] = graphics_queue_family_index;
            if (!present_shares_graphics_queue) indices[index++] = present_queue_family_index;
            if (!transfer_shares_graphics_queue) indices[index++] = transfer_queue_family_index;

            VkDeviceQueueCreateInfo queue_create_infos[4];
            f32 queue_priority = 1.0f;
            for (u32 i = 0; i < index_count; ++i) {
                queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_infos[i].queueFamilyIndex = indices[i];
                queue_create_infos[i].queueCount = 1;
                queue_create_infos[i].flags = 0;
                queue_create_infos[i].pNext = nullptr;
                queue_create_infos[i].pQueuePriorities = &queue_priority;
            }

            // Request device features.
            VkPhysicalDeviceFeatures device_features = {};
            device_features.samplerAnisotropy = VK_TRUE;  // Request anistropy

            bool portability_required = false;
            if (available_extensions_count) {
                for (u32 i = 0; i < available_extensions_count; ++i) {
                    if (areStringsEqual(available_extensions[i].extensionName, "VK_KHR_portability_subset")) {
                        SLIM_LOG_INFO("Adding required extension 'VK_KHR_portability_subset'.");
                        portability_required = true;
                        break;
                    }
                }
            }

            u32 extension_count = portability_required ? 2 : 1;
            const char* extension_names[2] = {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                "VK_KHR_portability_subset"
            };
            VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
            device_create_info.queueCreateInfoCount = index_count;
            device_create_info.pQueueCreateInfos = queue_create_infos;
            device_create_info.pEnabledFeatures = &device_features;
            device_create_info.enabledExtensionCount = extension_count;
            device_create_info.ppEnabledExtensionNames = extension_names;
            device_create_info.enabledLayerCount = _instance::enabled_validation_layers_count; // Deprecated but recommended to use instance-level for backwards compatibility
            device_create_info.ppEnabledLayerNames = _instance::enabled_validation_layer_names; // Deprecated but recommended to use instance-level for backwards compatibility

            // Create the device.
            VK_CHECK(vkCreateDevice(physical_device, &device_create_info, 0, &device));
            VK_SET_DEBUG_OBJECT_NAME(VK_OBJECT_TYPE_DEVICE, device, "Vulkan Logical Device");
            SLIM_LOG_INFO("Logical device created");

            // Get queues.
            vkGetDeviceQueue(device, graphics_queue_family_index, 0, &graphics_queue);
            vkGetDeviceQueue(device, present_queue_family_index, 0, &present_queue);
            vkGetDeviceQueue(device, transfer_queue_family_index, 0, &transfer_queue);
            SLIM_LOG_INFO("Queues obtained");

            return true;
        }

        i32 findMemoryIndex(u32 type_filter, u32 property_flags) {
            for (u32 i = 0; i < memory_properties.memoryTypeCount; ++i) {
                // Check each memory type to see if its bit is set to 1.
                if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & property_flags) == property_flags) {
                    return (i32)i;
                }
            }

            SLIM_LOG_WARNING("Unable to find suitable memory type!");
            return -1;
        }

        void destroy() {
            graphics_queue = nullptr;
            present_queue = nullptr;
            compute_queue = nullptr;
            transfer_queue = nullptr;
            graphics_queue_family_index = -1;
            present_queue_family_index = -1;
            compute_queue_family_index = -1;
            transfer_queue_family_index = -1;

            SLIM_LOG_INFO("Destroying logical device...");
            if (device) {
                vkDestroyDevice(device, nullptr);
                device = nullptr;
            }

            physical_device = 0;
        }
    }
}