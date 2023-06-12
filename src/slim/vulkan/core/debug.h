#pragma once

#include "./base.h"

namespace gpu {
    namespace _debug {
#ifdef NDEBUG
        bool init() { return true; }
        void shutdown() {}
#else
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
            debug_create_info.pfnUserCallback = VulkanDebugCallback;

            PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            //    KASSERT_MSG(func, "Failed to create debug messenger!");
            VK_CHECK(func(instance, &debug_create_info, 0, &messenger));
            SLIM_LOG_DEBUG("Vulkan debugger created.");

            // Load up debug function pointers.
            pfnSetObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
            if (!pfnSetObjectNameEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkSetDebugUtilsObjectNameEXT. Debug functions associated with this will not work.");
            }
            pfnSetObjectTagEXT = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectTagEXT");
            if (!pfnSetObjectTagEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkSetDebugUtilsObjectTagEXT. Debug functions associated with this will not work.");
            }

            pfnCmdBeginLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
            if (!pfnCmdBeginLabelEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkCmdBeginDebugUtilsLabelEXT. Debug functions associated with this will not work.");
            }

            pfnCmdEndLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
            if (!pfnCmdEndLabelEXT) {
                SLIM_LOG_WARNING("Unable to load function pointer for vkCmdEndDebugUtilsLabelEXT. Debug functions associated with this will not work.");
            }

            // If validation should be done, get a list of the required validation layer names
            // and make sure they exist. Validation layers should only be enabled on non-release builds.
            SLIM_LOG_INFO("Checking validation layers...");

            // Verify all required layers are available.
            for (u32 i = 0; i < _instance::enabled_validation_layers_count; ++i) {
                bool found = false;
                for (u32 j = 0; j < _instance::available_validation_layers_count; ++j) {
                    if (areStringsEqual(_instance::enabled_validation_layer_names[i], _instance::available_validation_layers[j].layerName)) {
                        found = true;
                        SLIM_LOG_INFO("Validation layer found: %s", _instance::enabled_validation_layer_names[i]);
                    }
                }

                if (!found) {
                    SLIM_LOG_FATAL("Missing validation layer: %s", _instance::enabled_validation_layer_names[i]);
                    return false;
                }
            }

            SLIM_LOG_INFO("Found all enabled validation layers");

            return true;
        }

        void shutdown() {
            SLIM_LOG_DEBUG("Destroying Vulkan debugger...");
            if (messenger) {
                PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
                func(instance, messenger, 0);
            }
        }
    }
#endif
}