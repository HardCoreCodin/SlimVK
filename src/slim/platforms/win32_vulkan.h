#pragma once

#include "./win32_base.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define VULKAN_WSI_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR &surface) {
    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    create_info.hinstance = Win32_instance;
    create_info.hwnd = Win32_window;

    VkResult result = vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface);
    if (result != VK_SUCCESS) {
        SLIM_LOG_FATAL("Vulkan surface creation failed.");
        return false;
    }

    return true;
}