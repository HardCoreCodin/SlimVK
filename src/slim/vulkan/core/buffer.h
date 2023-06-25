
#include "./base.h"

namespace gpu {
    bool Buffer::create(BufferType buffer_type, u64 size) {
        *this = {};
        type = buffer_type;
        total_size = size;

        switch (type) {
            case BufferType::Vertex:
                usage = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
                memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            case BufferType::Index:
                usage = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
                memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            case BufferType::Uniform: {
                u32 device_local_bits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // device.supports_device_local_host_visible ?  : 0;
                usage = (VkBufferUsageFlagBits)(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
                memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bits;
            } break;
            case BufferType::Staging:
                usage = (VkBufferUsageFlagBits)VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            case BufferType::Read:
                usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                memory_property_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                break;
            case BufferType::Storage:
                SLIM_LOG_ERROR("Storage buffer not yet supported.")
                return false;
            default:
                SLIM_LOG_ERROR("Unsupported buffer type: %i", type);
                return false;
        }

        VkBufferCreateInfo buffer_info = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = total_size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // NOTE: Only used in one queue.

        VK_CHECK(vkCreateBuffer(device, &buffer_info, nullptr, &handle))

        // Gather memory requirements.
        vkGetBufferMemoryRequirements(device, handle, &memory_requirements);
        memory_index = _device::getMemoryIndex(memory_requirements.memoryTypeBits, memory_property_flags);
        if (memory_index == -1) {
            SLIM_LOG_ERROR("Unable to create vulkan buffer because the required memory type index was not found.");
            return false;
        }

        // Allocate memory info
        VkMemoryAllocateInfo allocate_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = (u32)memory_index;

        VkResult result = vkAllocateMemory(device, &allocate_info, nullptr, &memory);

        if (result != VK_SUCCESS) {
            SLIM_LOG_ERROR("Unable to create vulkan buffer because the required memory allocation failed. Error: %i", result)
            return false;
        }

        VK_CHECK(vkBindBufferMemory(device, handle, memory, 0))

        return true;
    }

    void Buffer::destroy() {
        if (memory) {
            vkFreeMemory(device, memory, nullptr);
            memory = nullptr;
        }
        if (handle) {
            vkDestroyBuffer(device, handle, nullptr);
            handle = nullptr;
        }

        usage = {};
        is_locked = false;
    }

    bool Buffer::resize(u64 new_size) {
        Buffer new_buffer{};
        new_buffer.create(type, new_size);

        // Copy over the data.
        copyTo(new_buffer);

        // Make sure anything potentially using these is finished.
        // NOTE: We could use vkQueueWaitIdle here if we knew what queue this buffer would be used with...
        vkDeviceWaitIdle(device);

        // Destroy the old
        if (memory) {
            vkFreeMemory(device, memory, nullptr);
            memory = nullptr;
        }
        if (handle) {
            vkDestroyBuffer(device, handle, nullptr);
            handle = nullptr;
        }

        // Set new properties
        memory = new_buffer.memory;
        handle = new_buffer.handle;

        return true;
    }


    bool Buffer::flush(u64 size, u64 offset) const {
        // NOTE: If not host-coherent, flush the mapped memory range.
        if (!isHostCoherent()) {
            if (size == 0) size = total_size;

            VkMappedMemoryRange range = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
            range.memory = memory;
            range.offset = offset;
            range.size = size;
            VK_CHECK(vkFlushMappedMemoryRanges(device, 1, &range))
        }

        return true;
    }

    bool Buffer::download(void** to, u64 size, u64 from_offset) {
        if (size == 0) size = total_size;

        if (isDeviceLocal() && !isHostVisible()) {
            // NOTE: If a read buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
            // create the read buffer, copy data to it, then read from that buffer.

            // TODO: Make this read buffer an argument to this function so it can be reused(?)
            // Create a host-visible staging buffer to copy to. Mark it as the destination of the transfer.
            Buffer download_buffer{};
            if (!download_buffer.create(BufferType::Read, size)) {
                SLIM_LOG_ERROR("vulkan_buffer_read() - Failed to create read buffer.")
                return false;
            }

            copyTo(download_buffer, size, from_offset);
            download_buffer.read(*to, size);
            download_buffer.destroy();
        } else
            read(*to, size, from_offset);

        return true;
    }

    bool Buffer::upload(const void* from, u64 size, u64 to_offset) {
        if (size == 0) size = total_size;

        if (isDeviceLocal() && !isHostVisible()) {
            // NOTE: If a staging buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
            // create a staging buffer to load the data into first. Then copy from it to the target buffer.

            // Create a host-visible staging buffer to upload to. Mark it as the source of the transfer.
            Buffer staging{};
            if (!staging.create(BufferType::Staging, size)) {
                SLIM_LOG_ERROR("vulkan_buffer_load_range() - Failed to create staging buffer.");
                return false;
            }

            staging.write(from, size);
            staging.copyTo(*this, size, 0, to_offset);
            staging.destroy();
        } else
            write(from, size, to_offset);

        return true;
    }

    bool Buffer::copyTo(const Buffer &to_buffer, u64 size, u64 from_offset, u64 to_offset) const {
        if (size == 0) size = total_size;

        // TODO: Assuming queue and pool usage here. Might want dedicated queue.
        VkQueue queue = graphics_queue;
        vkQueueWaitIdle(queue);

        // Create a one-time-use command buffer.
        GraphicsCommandBuffer temp_command_buffer;
        graphics_command_pool.allocate(temp_command_buffer, true);
        temp_command_buffer.beginSingleUse();

        // Prepare the copy command and add it to the command buffer.
        VkBufferCopy copy_region;
        copy_region.srcOffset = from_offset;
        copy_region.dstOffset = to_offset;
        copy_region.size = size;
        vkCmdCopyBuffer(temp_command_buffer.handle, handle, to_buffer.handle, 1, &copy_region);
        temp_command_buffer.endSingleUse();

        return true;
    }

    void* Buffer::data(u64 size, u64 offset) const {
        void* data_ptr;
        VK_CHECK(vkMapMemory(device, memory, offset, size, 0, &data_ptr))
        return data_ptr;
    }

    void Buffer::read(void* to, u64 size, u64 from_offset) const {
        memcpy(to, data(size, from_offset), size);
        vkUnmapMemory(device, memory);
    }

    void Buffer::write(const void* from, u64 size, u64 to_offset) {
        memcpy(data(size, to_offset), from, size);
        vkUnmapMemory(device, memory);
    }
}
