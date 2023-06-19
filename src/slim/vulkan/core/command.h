#pragma once

#include "./base.h"

namespace gpu {
    void CommandBuffer::setViewport(const VkRect2D &rect) {
        VkViewport viewport;
        viewport.x = (float)rect.offset.x;
        viewport.y = (float)rect.offset.y;
        viewport.width = (float)rect.extent.width;
        viewport.height = (float)rect.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(handle, 0, 1, &viewport);
    }

    void CommandBuffer::setScissor(VkRect2D rect) {
        vkCmdSetScissor(handle, 0, 1, &rect);
    }

    void CommandBuffer::transitionImageLayout(Image &image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
        VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcQueueFamilyIndex = queue_family_index;
        barrier.dstQueueFamilyIndex = queue_family_index;
        barrier.image = image.handle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = image.type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags dest_stage;

        // Don't care about the old layout - transition to optimal layout (for the underlying implementation).
        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            // Don't care what stage the pipeline is in at the start.
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            // Used for copying
            dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // Transitioning from a transfer destination layout to a shader-readonly layout.
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // From a copying stage to...
            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            // The fragment stage.
            dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            // Transitioning from a transfer source layout to a shader-readonly layout.
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            // From a copying stage to...
            source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;

            // The fragment stage.
            dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            // Don't care what stage the pipeline is in at the start.
            source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            // Used for copying
            dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else {
            SLIM_LOG_FATAL("unsupported layout transition!");
            return;
        }

        vkCmdPipelineBarrier(handle, source_stage, dest_stage, 0,
                             0, 0,
                             0, 0,
                             1, &barrier);
    }

    void CommandBuffer::begin(bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use) {
        VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin_info.flags = 0;
        if (is_single_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (is_renderpass_continue) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        if (is_simultaneous_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VK_CHECK(vkBeginCommandBuffer(handle, &begin_info));
        state = State::Recording;
    }

    void CommandBuffer::end() {
        VK_CHECK(vkEndCommandBuffer(handle));
        state = State::Recorded;
    }

    void CommandBuffer::reset() {
        state = State::Ready;
    }

    void CommandBuffer::beginSingleUse() {
        begin(true, false, false);
    }

    void CommandBuffer::endSingleUse() {
        end();

        VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &handle;
        VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, nullptr));

        // Wait for it to finish
        VK_CHECK(vkQueueWaitIdle(queue));

        // Free the command buffer.
        free();
    }

    void CommandBuffer::free() {
        if (handle) vkFreeCommandBuffers(device, pool, 1, &handle);
        handle = nullptr;
        state = State::UnAllocated;
    }
}