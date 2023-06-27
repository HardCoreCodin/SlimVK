#pragma once

#include "./base.h"

namespace gpu {

    struct CommandBuffer {
        VkCommandBuffer handle;

    protected:
        const VkCommandPool pool;
        const VkQueue queue;
        const unsigned int queue_family_index;

    public:
        State state = State::Ready;

        explicit CommandBuffer(
            VkCommandBuffer buffer = nullptr,
            VkCommandPool pool = nullptr,
            VkQueue queue = nullptr,
            unsigned int queue_family_index = 0) :

            handle{buffer},
            pool{pool},
            queue{queue},
            queue_family_index{queue_family_index} {
        }

        void begin(bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use) {
            VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            begin_info.flags = 0;
            if (is_single_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            if (is_renderpass_continue) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
            if (is_simultaneous_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            VK_CHECK(vkBeginCommandBuffer(handle, &begin_info));
            state = State::Recording;
        }

        void end() {
            VK_CHECK(vkEndCommandBuffer(handle));
            state = State::Recorded;
        }

        void reset() {
            state = State::Ready;
        }

        void beginSingleUse() {
            begin(true, false, false);
        }

        void endSingleUse() {
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

        void free() {
            if (handle) vkFreeCommandBuffers(device, pool, 1, &handle);
            handle = nullptr;
            state = State::UnAllocated;
        }

        void copy(const Buffer &from_buffer, const Image &to_image) const {
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = to_image.type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

            region.imageExtent.width = to_image.width;
            region.imageExtent.height = to_image.height;
            region.imageExtent.depth = 1;

            vkCmdCopyBufferToImage(handle, from_buffer.handle, to_image.handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1,&region);
        }

        void copy(const Image &from_image, const Buffer &to_buffer) const {
            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = from_image.type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

            region.imageExtent.width = from_image.width;
            region.imageExtent.height = from_image.height;
            region.imageExtent.depth = 1;

            vkCmdCopyImageToBuffer(handle, from_image.handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   to_buffer.handle, 1,&region);
        }

        void copy(const Image &from_image,  u32 x, u32 y, const Buffer &to_buffer) const {
            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = from_image.type == VK_IMAGE_VIEW_TYPE_CUBE ? 6 : 1;

            region.imageOffset.x = x;
            region.imageOffset.y = y;
            region.imageExtent.width = 1;
            region.imageExtent.height = 1;
            region.imageExtent.depth = 1;

            vkCmdCopyImageToBuffer(handle, from_image.handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   to_buffer.handle, 1, &region);
        }

        bool copy(const Buffer &from_buffer, const Buffer &to_buffer, u64 size = 0, u64 from_offset = 0, u64 to_offset = 0) const {
            if (size == 0) size = from_buffer.total_size;

            vkQueueWaitIdle(queue);

            // Create a one-time-use command buffer.
            CommandBuffer temp_command_buffer;
            VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            allocate_info.commandPool = pool;
            allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocate_info.commandBufferCount = 1;
            VkCommandBuffer command_buffer_handle;
            VK_CHECK(vkAllocateCommandBuffers(device, &allocate_info, &command_buffer_handle))
            new(&temp_command_buffer)CommandBuffer(command_buffer_handle, pool, queue, queue_family_index);
            temp_command_buffer.reset();

            temp_command_buffer.beginSingleUse();

            // Prepare the copy command and add it to the command buffer.
            VkBufferCopy copy_region;
            copy_region.srcOffset = from_offset;
            copy_region.dstOffset = to_offset;
            copy_region.size = size;
            vkCmdCopyBuffer(temp_command_buffer.handle, from_buffer.handle, to_buffer.handle, 1, &copy_region);

            temp_command_buffer.endSingleUse();

            return true;
        }

        bool resize(Buffer &buffer, u64 new_size) const {
            Buffer new_buffer{};
            new_buffer.create(buffer.type, new_size);

            // Copy over the data.
            copy(buffer, new_buffer);

            // Make sure anything potentially using these is finished.
            // NOTE: We could use vkQueueWaitIdle here if we knew what queue this buffer would be used with...
            vkDeviceWaitIdle(device);

            // Destroy the old
            if (buffer.memory) {
                vkFreeMemory(device, buffer.memory, nullptr);
                buffer.memory = nullptr;
            }
            if (buffer.handle) {
                vkDestroyBuffer(device, buffer.handle, nullptr);
                buffer.handle = nullptr;
            }

            // Set new properties
            buffer.memory = new_buffer.memory;
            buffer.handle = new_buffer.handle;

            return true;
        }

        bool download(const Buffer &from_buffer, void** to_data, u64 size = 0, u64 from_offset = 0) const {
            if (size == 0) size = from_buffer.total_size;

            if (from_buffer.isDeviceLocal() && !from_buffer.isHostVisible()) {
                // NOTE: If a read buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
                // create the read buffer, copy data to it, then read from that buffer.

                // TODO: Make this read buffer an argument to this function so it can be reused(?)
                // Create a host-visible staging buffer to copy to. Mark it as the destination of the transfer.
                Buffer download_buffer{};
                if (!download_buffer.create(BufferType::Read, size)) {
                    SLIM_LOG_ERROR("vulkan_buffer_read() - Failed to create read buffer.")
                    return false;
                }

                copy(from_buffer, download_buffer, size, from_offset);
                download_buffer.read(*to_data, size);
                download_buffer.destroy();
            } else
                from_buffer.read(*to_data, size, from_offset);

            return true;
        }

        bool upload(const void* from_data, const Buffer &to_buffer, u64 size = 0, u64 to_offset = 0) const {
            if (size == 0) size = to_buffer.total_size;

            if (to_buffer.isDeviceLocal() && !to_buffer.isHostVisible()) {
                // NOTE: If a staging buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
                // create a staging buffer to load the data into first. Then copy from it to the target buffer.

                // Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
                Buffer staging{};
                if (!staging.create(BufferType::Staging, size)) {
                    SLIM_LOG_ERROR("vulkan_buffer_load_range() - Failed to create staging buffer.");
                    return false;
                }

                staging.write(from_data, size);
                copy(staging, to_buffer, size, 0, to_offset);
                staging.destroy();
            } else
                to_buffer.write(from_data, size, to_offset);

            return true;
        }

        void transitionImageLayout(Image &image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) const {
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
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);
        }
    };

    template <typename CommandBufferType>
    struct CommandPool {
        VkCommandPool handle;

        void free(CommandBufferType &command_buffer) {
            vkFreeCommandBuffers(device, handle,1, &command_buffer.handle);
            command_buffer.handle = 0;
            command_buffer.state = State::UnAllocated;
        }

        void destroy() {
            vkDestroyCommandPool(device, handle, nullptr);
        }

        void allocate(CommandBufferType &command_buffer, bool is_primary = true) {
            if (command_buffer.handle) command_buffer.free();

            VkCommandBufferAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            allocate_info.commandPool = handle;
            allocate_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            allocate_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer_handle;
            VK_CHECK(vkAllocateCommandBuffers(device, &allocate_info, &command_buffer_handle))
            new(&command_buffer)CommandBufferType(command_buffer_handle, handle);
            command_buffer.reset();
        }

    protected:
        void _create(unsigned int queue_family_index, bool transient) {
            // Create command pool for graphics queue.
            VkCommandPoolCreateInfo pool_create_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
            pool_create_info.queueFamilyIndex = queue_family_index;
            pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            if (transient)
                pool_create_info.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

            VK_CHECK(vkCreateCommandPool(device, &pool_create_info, nullptr, &handle))
        }
    };
}