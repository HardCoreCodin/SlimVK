#pragma once

#include "./base.h"

namespace gpu {
    struct CommandBuffer {
        VkCommandBuffer handle;
        const VkPipelineBindPoint bind_point;
        const VkCommandPool pool;
        const VkQueue queue;
        const unsigned int queue_family_index;

        mutable State state = State::Ready;

        explicit CommandBuffer(
            VkCommandBuffer buffer = nullptr,
            VkCommandPool pool = nullptr,
            VkQueue queue = nullptr,
            unsigned int queue_family_index = 0,
            VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS) :
            handle{buffer},
            bind_point{bind_point},
            pool{pool},
            queue{queue},
            queue_family_index{queue_family_index} {
        }

        void begin(bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use) const {
            VkCommandBufferBeginInfo begin_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            begin_info.flags = 0;
            if (is_single_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            if (is_renderpass_continue) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
            if (is_simultaneous_use) begin_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

            VK_CHECK(vkBeginCommandBuffer(handle, &begin_info));
            state = State::Recording;
        }

        void end() const {
            VK_CHECK(vkEndCommandBuffer(handle));
            state = State::Recorded;
        }

        void reset() const {
            state = State::Ready;
        }

        void beginSingleUse() const {
            begin(true, false, false);
        }

        void endSingleUse() const {
            end();

            VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &handle;
            VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, nullptr));
            VK_CHECK(vkQueueWaitIdle(queue));
        }

        void free() {
            if (handle) vkFreeCommandBuffers(device, pool, 1, &handle);
            handle = nullptr;
            state = State::UnAllocated;
        }
    };

    template <typename CommandBufferType>
    struct CommandPool {
        VkCommandPool handle = nullptr;

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