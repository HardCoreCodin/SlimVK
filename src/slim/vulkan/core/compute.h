#pragma once

#include "./command.h"

namespace gpu {
    struct ComputeCommandBuffer : CommandBuffer {
        ComputeCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
            CommandBuffer(command_buffer_handle, command_pool, compute_queue, compute_queue_family_index, VK_PIPELINE_BIND_POINT_COMPUTE) {}
    };

    struct ComputeCommandPool : CommandPool<ComputeCommandBuffer> {
        void create(bool transient = false) {
            _create(compute_queue_family_index, transient);
            SLIM_LOG_INFO("Compute command pool created.")
        }
    };
}