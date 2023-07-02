#pragma once

#include "./base.h"

namespace gpu {


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

        void create(VkImageViewType view_type,
                    u32 image_width,
                    u32 image_height,
                    VkFormat format,
                    VkImageTiling tiling,
                    VkImageUsageFlags usage,
                    VkMemoryPropertyFlags image_memory_flags,
                    VkImageAspectFlags view_aspect_flags,
                    const char* image_name = nullptr,
                    bool create_view = true) {
            // Copy params
            width = image_width;
            height = image_height;
            memory_flags = image_memory_flags;
            name = (char*)image_name;//string_duplicate(image_name);
            type = view_type;

            // Creation info.
            VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};

            image_create_info.imageType = VK_IMAGE_TYPE_2D;
            image_create_info.extent.width = width;
            image_create_info.extent.height = height;
            image_create_info.extent.depth = 1;                                 // TODO: Support configurable depth.
            image_create_info.mipLevels = 4;                                    // TODO: Support mip mapping
            image_create_info.arrayLayers = type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;  // TODO: Support number of layers in the image.
            image_create_info.format = format;
            image_create_info.tiling = tiling;
            image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_create_info.usage = usage;
            image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;          // TODO: Configurable sample count.
            image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // TODO: Configurable sharing mode.
            if (type == VK_IMAGE_VIEW_TYPE_CUBE) {
                image_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
            }

            VK_CHECK(vkCreateImage(device, &image_create_info, nullptr, &handle));
            VK_SET_DEBUG_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE, handle, name);

            // Query memory requirements.
            vkGetImageMemoryRequirements(device, handle, &memory_requirements);
            i32 memory_type = _device::getMemoryIndex(memory_requirements.memoryTypeBits, memory_flags);
            if (memory_type == -1) SLIM_LOG_ERROR("Required memory type not found. Image not valid.");

            // Allocate memory
            VkMemoryAllocateInfo memory_allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
            memory_allocate_info.allocationSize = memory_requirements.size;
            memory_allocate_info.memoryTypeIndex = memory_type;
            VK_CHECK(vkAllocateMemory(device, &memory_allocate_info, nullptr, &memory));

            // Bind the memory
            VK_CHECK(vkBindImageMemory(device, handle, memory, 0))  // TODO: configurable memory offset.

            // Create view
            if (create_view) {
                view = nullptr;
                createView(type, format, view_aspect_flags);
            }
        }

        void createView(VkImageViewType view_type, VkFormat format, VkImageAspectFlags aspect_flags) {
            VkImageViewCreateInfo view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            view_create_info.image = handle;
            view_create_info.viewType = view_type;
            view_create_info.subresourceRange.layerCount = view_type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;
            view_create_info.format = format;
            view_create_info.subresourceRange.aspectMask = aspect_flags;

            // TODO: Make configurable
            view_create_info.subresourceRange.baseMipLevel = 0;
            view_create_info.subresourceRange.levelCount = 1;
            view_create_info.subresourceRange.baseArrayLayer = 0;
            view_create_info.subresourceRange.layerCount = 1;

            VK_CHECK(vkCreateImageView(device, &view_create_info, nullptr, &view))

            char formatted_name[TEXTURE_NAME_MAX_LENGTH + 5] = {0};
            int i = 0;
            while (name[i]) {
                formatted_name[i] = name[i];
                i++;
            }
            formatted_name[i++] = '_';
            formatted_name[i++] = 'v';
            formatted_name[i++] = 'i';
            formatted_name[i++] = 'e';
            formatted_name[i++] = 'w';
            formatted_name[i] = '\0';
            VK_SET_DEBUG_OBJECT_NAME(VK_OBJECT_TYPE_IMAGE_VIEW, view, formatted_name);
        }

        void destroy() {
            if (view) {
                vkDestroyImageView(device, view, nullptr);
                view = nullptr;
            }
            if (memory) {
                vkFreeMemory(device, memory, nullptr);
                memory = nullptr;
            }
            if (handle) {
                vkDestroyImage(device, handle, nullptr);
                handle = nullptr;
            }
        }
    };
}
