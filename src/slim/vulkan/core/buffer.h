#pragma once

#include "./base.h"
#include "./command.h"

namespace gpu {
    struct Buffer {
        BufferType type;

        VkBuffer handle;
        VkBufferUsageFlagBits usage;

        VkDeviceMemory memory;
        VkMemoryRequirements memory_requirements;
        i32 memory_index;
        u32 memory_property_flags;
        u64 total_size;

        INLINE bool isDeviceLocal() const { return (memory_property_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; }
        INLINE bool isHostVisible() const { return (memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; }
        INLINE bool isHostCoherent() const { return (memory_property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; }

        bool create(BufferType buffer_type, u64 size) {
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

        bool createAsStaging(u64 size, const void *data) {
            if (!Buffer::create(BufferType::Staging, size)) {
                SLIM_LOG_ERROR("Failed to create staging buffer")
                return false;
            }
            write(data, size);
            return true;
        }

        void destroy() {
            if (memory) {
                vkFreeMemory(device, memory, nullptr);
                memory = nullptr;
            }
            if (handle) {
                vkDestroyBuffer(device, handle, nullptr);
                handle = nullptr;
            }

            usage = {};
        }

        bool flush(u64 size = 0, u64 offset = 0) const {
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

        bool copyTo(const Buffer &to_buffer, const CommandBuffer &temp_command_buffer, u64 size = 0, u64 from_offset = 0, u64 to_offset = 0) const {
            if (size == 0) size = total_size;

            vkQueueWaitIdle(temp_command_buffer.queue);

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

        bool resize(u64 new_size, const CommandBuffer &command_buffer) {
            Buffer new_buffer{};
            new_buffer.create(type, new_size);

            // Copy over the data.
            copyTo(new_buffer, command_buffer);

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

        bool download(void** to_data, const CommandBuffer &command_buffer, u64 size = 0, u64 from_offset = 0) const {
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

                copyTo(download_buffer, command_buffer, size, from_offset);
                download_buffer.read(*to_data, size);
                download_buffer.destroy();
            } else
                read(*to_data, size, from_offset);

            return true;
        }

        bool upload(const void* from_data, const CommandBuffer &command_buffer, u64 size = 0, u64 to_offset = 0) const {
            if (size == 0) size = total_size;

            if (isDeviceLocal() && !isHostVisible()) {
                // NOTE: If a staging buffer is needed (i.e.) the target buffer's memory is not host visible but is device-local,
                // create a staging buffer to load the data into first. Then copy from it to the target buffer.

                Buffer staging{};
                staging.createAsStaging(size, from_data);
                staging.copyTo(*this, command_buffer, size, 0, to_offset);
                staging.destroy();
            } else
                write(from_data, size, to_offset);

            return true;
        }

    protected:
        void* data(u64 size, u64 offset = 0) const {
            void* data_ptr;
            VK_CHECK(vkMapMemory(device, memory, offset, size, 0, &data_ptr))
            return data_ptr;
        }

        void read(void* to, u64 size, u64 from_offset = 0) const {
            memcpy(to, data(size, from_offset), size);
            vkUnmapMemory(device, memory);
        }

        void write(const void* from, u64 size, u64 to_offset = 0) const {
            memcpy(data(size, to_offset), from, size);
            vkUnmapMemory(device, memory);
        }
    };
}
