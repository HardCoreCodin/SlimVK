#pragma once

#include "./base.h"

namespace gpu {
    namespace _instance {
        bool init(const char* app_name = "SlimVK") {
            available_validation_layers_count = 0;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&available_validation_layers_count, 0));
            VK_CHECK(vkEnumerateInstanceLayerProperties(&available_validation_layers_count, available_validation_layers));

            available_extensions_count = 0;
            vkEnumerateInstanceExtensionProperties(0, &available_extensions_count, 0);
            vkEnumerateInstanceExtensionProperties(0, &available_extensions_count, available_extensions);

            if (!allEnabledExtensionsAreAvailable(enabled_extension_names, enabled_extensions_count,
                                                  available_extensions, available_extensions_count)) {
                SLIM_LOG_FATAL("Required extension(s) are missing! Aborting...");
                return false;
            }

            VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
            app_info.apiVersion = VK_API_VERSION_1_2;
            app_info.pApplicationName = app_name;
            app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            app_info.pEngineName = "SlimEngine";
            app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);

            VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
            create_info.pApplicationInfo = &app_info;
            create_info.enabledExtensionCount = enabled_extensions_count;
            create_info.ppEnabledExtensionNames = enabled_extension_names;
            create_info.enabledLayerCount = enabled_validation_layers_count;
            create_info.ppEnabledLayerNames = enabled_validation_layer_names;

            // #if 'ON_APPLE'
            //create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

            VkResult instance_result = vkCreateInstance(&create_info, nullptr, &instance);
            if (!isVulkanResultSuccess(instance_result)) {
                const char* result_string = getVulkanResultString(instance_result, true);
                SLIM_LOG_FATAL("Vulkan instance creation failed with result: '%s'", result_string);
                return false;
            }

            SLIM_LOG_INFO("Vulkan Instance created");
            return true;
        }
    }
}