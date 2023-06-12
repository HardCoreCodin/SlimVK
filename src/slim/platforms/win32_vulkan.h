#pragma once

#include "./win32_base.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define VK_WSI_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"

VkResult CreateVulkanSurface(VkInstance instance, VkSurfaceKHR &surface) {
    VkWin32SurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    create_info.hinstance = Win32_instance;
    create_info.hwnd = Win32_window;

    return vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface);
}