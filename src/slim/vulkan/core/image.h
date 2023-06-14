#pragma once

#include "./base.h"


void gpu::Image::create(TextureType type, u32 image_width, u32 image_height, VkFormat format, VkImageTiling tiling,
                        VkImageUsageFlags usage, VkMemoryPropertyFlags image_memory_flags, bool create_view,
                        VkImageAspectFlags view_aspect_flags, const char* image_name) {
    // Copy params
    width = image_width;
    height = image_height;
    memory_flags = image_memory_flags;
    name = (char*)image_name;//string_duplicate(image_name);

    // Creation info.
    VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    switch (type) {
        default:
        case TextureType_2D:
        case TextureType_CubeMap:  // Intentional, there is no cube image type.
            image_create_info.imageType = VK_IMAGE_TYPE_2D;
            break;
    }

    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;                                 // TODO: Support configurable depth.
    image_create_info.mipLevels = 4;                                    // TODO: Support mip mapping
    image_create_info.arrayLayers = type == TextureType_CubeMap ? 6 : 1;  // TODO: Support number of layers in the image.
    image_create_info.format = format;
    image_create_info.tiling = tiling;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage = usage;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;          // TODO: Configurable sample count.
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // TODO: Configurable sharing mode.
    if (type == TextureType_CubeMap) {
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

    // Report the memory as in-use.
//    bool is_device_memory = (memory_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//    kallocate_report(memory_requirements.size, is_device_memory ? MEMORY_TAG_GPU_LOCAL : MEMORY_TAG_VULKAN);

    // Create view
    if (create_view) {
        view = 0;
        createView(type, format, view_aspect_flags);
    }
}

void gpu::Image::createView(TextureType type, VkFormat format, VkImageAspectFlags aspect_flags) {
    VkImageViewCreateInfo view_create_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_create_info.image = handle;
    switch (type) {
        case TextureType_CubeMap:
            view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        default:
        case TextureType_2D:
            view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
    }
    view_create_info.format = format;
    view_create_info.subresourceRange.aspectMask = aspect_flags;

    // TODO: Make configurable
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = type == TextureType_CubeMap ? 6 : 1;

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

void gpu::Image::destroy() {
    if (view) {
        vkDestroyImageView(device, view, nullptr);
        view = 0;
    }
    if (memory) {
        vkFreeMemory(device, memory, nullptr);
        memory = 0;
    }
    if (handle) {
        vkDestroyImage(device, handle, nullptr);
        handle = 0;
    }
//    if (name) {
//        kfree(name, string_length(name) + 1, MEMORY_TAG_STRING);
//        name = 0;
//    }

    // Report the memory as no longer in-use.
//    bool is_device_memory = (memory_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
//    kfree_report(image->memory_requirements.size, is_device_memory ? MEMORY_TAG_GPU_LOCAL : MEMORY_TAG_VULKAN);
//    kzero_memory(&image->memory_requirements, sizeof(VkMemoryRequirements));
}

//
//    void transitionLayout(
////            texture_type type,
////            vulkan_command_buffer* command_buffer,
//        VkFormat format,
//        VkImageLayout old_layout,
//        VkImageLayout new_layout);
//
//    void copyFromBuffer(
////            texture_type type,
//        VkBuffer buffer
////            vulkan_command_buffer* command_buffer
//    );
//
//    void copyToBuffer(
////            texture_type type,
//        VkBuffer buffer
////            vulkan_command_buffer* command_buffer
//    );
//
//    void copyPixelToBuffer(
////            texture_type type,
//        VkBuffer buffer,
//        u32 x,
//        u32 y
////            vulkan_command_buffer* command_buffer
//    );

//
//
//void vulkan_image_transition_layout(
//    vulkan_context* context,
//    texture_type type,
//    vulkan_command_buffer* command_buffer,
//    vulkan_image* image,
//    VkFormat format,
//    VkImageLayout old_layout,
//    VkImageLayout new_layout) {
//    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
//    barrier.oldLayout = old_layout;
//    barrier.newLayout = new_layout;
//    barrier.srcQueueFamilyIndex = context->device.graphics_queue_index;
//    barrier.dstQueueFamilyIndex = context->device.graphics_queue_index;
//    barrier.image = image->handle;
//    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    barrier.subresourceRange.baseMipLevel = 0;
//    barrier.subresourceRange.levelCount = 1;
//    barrier.subresourceRange.baseArrayLayer = 0;
//    barrier.subresourceRange.layerCount = type == TEXTURE_TYPE_CUBE ? 6 : 1;
//
//    VkPipelineStageFlags source_stage;
//    VkPipelineStageFlags dest_stage;
//
//    // Don't care about the old layout - transition to optimal layout (for the underlying implementation).
//    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
//        barrier.srcAccessMask = 0;
//        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//
//        // Don't care what stage the pipeline is in at the start.
//        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//
//        // Used for copying
//        dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
//        // Transitioning from a transfer destination layout to a shader-readonly layout.
//        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//        // From a copying stage to...
//        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//
//        // The fragment stage.
//        dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
//        // Transitioning from a transfer source layout to a shader-readonly layout.
//        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//        // From a copying stage to...
//        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//
//        // The fragment stage.
//        dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//    } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
//        barrier.srcAccessMask = 0;
//        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//
//        // Don't care what stage the pipeline is in at the start.
//        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//
//        // Used for copying
//        dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//    } else {
//        KFATAL("unsupported layout transition!");
//        return;
//    }
//
//    vkCmdPipelineBarrier(
//        command_buffer->handle,
//        source_stage, dest_stage,
//        0,
//        0, 0,
//        0, 0,
//        1, &barrier);
//}
//
//void vulkan_image_copy_from_buffer(
//    vulkan_context* context,
//    texture_type type,
//    vulkan_image* image,
//    VkBuffer buffer,
//    vulkan_command_buffer* command_buffer) {
//    // Region to copy
//    VkBufferImageCopy region;
//    kzero_memory(&region, sizeof(VkBufferImageCopy));
//    region.bufferOffset = 0;
//    region.bufferRowLength = 0;
//    region.bufferImageHeight = 0;
//
//    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    region.imageSubresource.mipLevel = 0;
//    region.imageSubresource.baseArrayLayer = 0;
//    region.imageSubresource.layerCount = type == TEXTURE_TYPE_CUBE ? 6 : 1;
//
//    region.imageExtent.width = image->width;
//    region.imageExtent.height = image->height;
//    region.imageExtent.depth = 1;
//
//    vkCmdCopyBufferToImage(
//        command_buffer->handle,
//        buffer,
//        image->handle,
//        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//        1,
//        &region);
//}
//
//void vulkan_image_copy_to_buffer(
//    vulkan_context* context,
//    texture_type type,
//    vulkan_image* image,
//    VkBuffer buffer,
//    vulkan_command_buffer* command_buffer) {
//    VkBufferImageCopy region = {};
//    region.bufferOffset = 0;
//    region.bufferRowLength = 0;
//    region.bufferImageHeight = 0;
//
//    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    region.imageSubresource.mipLevel = 0;
//    region.imageSubresource.baseArrayLayer = 0;
//    region.imageSubresource.layerCount = type == TEXTURE_TYPE_CUBE ? 6 : 1;
//
//    region.imageExtent.width = image->width;
//    region.imageExtent.height = image->height;
//    region.imageExtent.depth = 1;
//
//    vkCmdCopyImageToBuffer(
//        command_buffer->handle,
//        image->handle,
//        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//        buffer,
//        1,
//        &region);
//}
//
//void vulkan_image_copy_pixel_to_buffer(
//    vulkan_context* context,
//    texture_type type,
//    vulkan_image* image,
//    VkBuffer buffer,
//    u32 x,
//    u32 y,
//    vulkan_command_buffer* command_buffer) {
//    VkBufferImageCopy region = {};
//    region.bufferOffset = 0;
//    region.bufferRowLength = 0;
//    region.bufferImageHeight = 0;
//
//    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    region.imageSubresource.mipLevel = 0;
//    region.imageSubresource.baseArrayLayer = 0;
//    region.imageSubresource.layerCount = type == TEXTURE_TYPE_CUBE ? 6 : 1;
//
//    region.imageOffset.x = x;
//    region.imageOffset.y = y;
//    region.imageExtent.width = 1;
//    region.imageExtent.height = 1;
//    region.imageExtent.depth = 1;
//
//    vkCmdCopyImageToBuffer(
//        command_buffer->handle,
//        image->handle,
//        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//        buffer,
//        1,
//        &region);
//}