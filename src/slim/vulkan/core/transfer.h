#pragma once

#include "./base.h"

namespace gpu {
    void TransferCommandBuffer::copyBufferToImage(VkBuffer buffer, Image &image) {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = image.type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

        region.imageExtent.width = image.width;
        region.imageExtent.height = image.height;
        region.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(handle, buffer, image.handle,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,&region);
    }

    void TransferCommandBuffer::copyImageToBuffer(Image &image, VkBuffer buffer) {
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = image.type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

        region.imageExtent.width = image.width;
        region.imageExtent.height = image.height;
        region.imageExtent.depth = 1;

        vkCmdCopyImageToBuffer(handle, image.handle,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               buffer, 1,&region);
    }

    void TransferCommandBuffer::copyPixelToBuffer(Image &image,  u32 x, u32 y, VkBuffer buffer) {
            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = image.type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

            region.imageOffset.x = x;
            region.imageOffset.y = y;
            region.imageExtent.width = 1;
            region.imageExtent.height = 1;
            region.imageExtent.depth = 1;

            vkCmdCopyImageToBuffer(handle, image.handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   buffer, 1, &region);
    }
}