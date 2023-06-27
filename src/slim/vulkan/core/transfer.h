#pragma once

#include "./command.h"

namespace gpu {
    struct TransferCommandBuffer : CommandBuffer {
        TransferCommandBuffer(VkCommandBuffer command_buffer_handle = nullptr, VkCommandPool command_pool = nullptr) :
            CommandBuffer(command_buffer_handle, command_pool, transfer_queue, transfer_queue_family_index) {}
    };

    struct TransferCommandPool : CommandPool<TransferCommandBuffer> {
        void create(bool transient = false) {
            _create(transfer_queue_family_index, transient);
            SLIM_LOG_INFO("Transfer command pool created.")
        }
    };

    TransferCommandPool transfer_command_pool;
    TransferCommandPool transient_transfer_command_pool;
}